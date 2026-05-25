const express = require('express');
const multer = require('multer');
const { exec } = require('child_process');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 3000;

// Ensure directories exist
const uploadDir = path.join(__dirname, 'data', 'uploads');
if (!fs.existsSync(uploadDir)) {
    fs.mkdirSync(uploadDir, { recursive: true });
}

// Multer storage configuration
const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, uploadDir);
    },
    filename: function (req, file, cb) {
        const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1e9);
        cb(null, uniqueSuffix + path.extname(file.originalname));
    }
});

const upload = multer({ storage: storage });

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));
app.use('/data', express.static(path.join(__dirname, 'data')));

// Helper to run a command and return stdout/stderr
function runCommand(command) {
    return new Promise((resolve) => {
        exec(command, (error, stdout, stderr) => {
            resolve({
                error,
                stdout: stdout || '',
                stderr: stderr || ''
            });
        });
    });
}

// POST /api/process endpoint
app.post('/api/process', upload.single('image'), async (req, res) => {
    if (!req.file) {
        return res.status(400).json({ error: 'No image file provided.' });
    }

    const inputPath = req.file.path;
    const inputFilename = req.file.filename;
    const baseName = path.parse(inputFilename).name;
    
    // We will save outputs as PNG since our C++ code writes PNG
    const getOutputPath = (mode) => path.join(uploadDir, `output_${mode}_${baseName}.png`);
    const getWebUrl = (mode) => `/data/uploads/output_${mode}_${baseName}.png`;

    const results = {
        originalUrl: `/data/uploads/${inputFilename}`,
        methods: {}
    };

    // Sequential runs to ensure accurate timing
    const runConfigs = [
        {
            name: 'serial',
            displayName: 'Serial Baseline',
            cmd: `./build/sobel_serial "${inputPath}" "${getOutputPath('serial')}" serial`,
            parseTime: (stdout) => {
                const match = stdout.match(/Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            }
        },
        {
            name: 'openmp',
            displayName: 'OpenMP (4 threads)',
            cmd: `./build/sobel_openmp "${inputPath}" "${getOutputPath('openmp')}" openmp 4`,
            parseTime: (stdout) => {
                const match = stdout.match(/Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            }
        },
        {
            name: 'pthreads',
            displayName: 'Pthreads (4 threads)',
            cmd: `./build/sobel_pthreads "${inputPath}" "${getOutputPath('pthreads')}" pthreads 4`,
            parseTime: (stdout) => {
                const match = stdout.match(/Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            }
        },
        {
            name: 'mpi',
            displayName: 'MPI (4 processes)',
            cmd: `mpirun --allow-run-as-root -np 4 ./build/sobel_mpi "${inputPath}" "${getOutputPath('mpi')}" "${getOutputPath('serial')}"`,
            parseTime: (stdout) => {
                const match = stdout.match(/MPI Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            },
            parseMetrics: (stdout) => {
                const rmseMatch = stdout.match(/RMSE vs reference:\s+([\d.]+)/);
                const psnrMatch = stdout.match(/PSNR:\s+([^\s]+)/);
                return {
                    rmse: rmseMatch ? parseFloat(rmseMatch[1]) : null,
                    psnr: psnrMatch ? psnrMatch[1].replace(/,/, '') : null
                };
            }
        },
        {
            name: 'hybrid',
            displayName: 'Hybrid (2 ranks x 2 threads)',
            cmd: `OMP_NUM_THREADS=2 mpirun --allow-run-as-root -np 2 ./build/sobel_hybrid "${inputPath}" "${getOutputPath('hybrid')}" "${getOutputPath('serial')}"`,
            parseTime: (stdout) => {
                const match = stdout.match(/Hybrid Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            },
            parseMetrics: (stdout) => {
                const rmseMatch = stdout.match(/RMSE vs reference:\s+([\d.]+)/);
                const psnrMatch = stdout.match(/PSNR:\s+([^\s]+)/);
                const computeMatch = stdout.match(/\(compute phase:\s+([\d.]+)\s+ms\)/);
                return {
                    rmse: rmseMatch ? parseFloat(rmseMatch[1]) : null,
                    psnr: psnrMatch ? psnrMatch[1].replace(/,/, '') : null,
                    computeTimeMs: computeMatch ? parseFloat(computeMatch[1]) : null
                };
            }
        },
        {
            name: 'cuda',
            displayName: 'CUDA GPU',
            cmd: `./build/sobel_cuda "${inputPath}" "${getOutputPath('cuda')}" cuda`,
            parseTime: (stdout) => {
                const match = stdout.match(/Execution time:\s+([\d.]+)\s+ms/);
                return match ? parseFloat(match[1]) : null;
            }
        }
    ];

    // Detect image dimensions from Serial baseline stdout
    let imageWidth = 0;
    let imageHeight = 0;

    for (const config of runConfigs) {
        console.log(`Running ${config.displayName} (${config.name})...`);
        const { error, stdout, stderr } = await runCommand(config.cmd);
        
        const timeMs = config.parseTime(stdout);
        const outputPath = getOutputPath(config.name);
        
        // A run is successful if the command exited 0, parsed execution time, and output image exists
        const fileExists = fs.existsSync(outputPath);
        const baseSuccess = !error && timeMs !== null && fileExists;
        
        let emulated = false;
        let success = baseSuccess;
        
        // For CUDA, if we succeeded but printed warning logs of fallback, mark it as emulated
        if (config.name === 'cuda' && baseSuccess) {
            const hasWarning = stderr.includes('WARNING') || stdout.includes('WARNING') || 
                              stderr.includes('fallback') || stdout.includes('fallback') ||
                              stderr.includes('failed') || stdout.includes('failed');
            if (hasWarning) {
                emulated = true;
            }
        }

        // If we are on serial, parse width and height
        if (config.name === 'serial' && baseSuccess) {
            const dimMatch = stdout.match(/Loaded image:\s+(\d+)x(\d+)/);
            if (dimMatch) {
                imageWidth = parseInt(dimMatch[1]);
                imageHeight = parseInt(dimMatch[2]);
            }
        }

        if (!success) {
            console.error(`Failed to run ${config.name}:`, error || stderr || stdout);
            results.methods[config.name] = {
                displayName: config.displayName,
                success: false,
                emulated: false,
                error: (stderr || stdout || (error ? error.message : 'Unknown error')).trim(),
                timeMs: null,
                speedup: null,
                outputUrl: null
            };
        } else {
            const metrics = config.parseMetrics ? config.parseMetrics(stdout) : {};
            results.methods[config.name] = {
                displayName: config.displayName + (emulated ? " (CPU Emulated)" : ""),
                success: true,
                emulated: emulated,
                timeMs: timeMs,
                speedup: null, // calculated below
                outputUrl: getWebUrl(config.name),
                error: emulated ? (stderr || stdout).trim() : null,
                ...metrics
            };
        }
    }

    results.dimensions = { width: imageWidth, height: imageHeight };

    // Calculate speedups relative to serial
    const serialTime = results.methods['serial'] && results.methods['serial'].success ? results.methods['serial'].timeMs : null;
    if (serialTime) {
        for (const mode in results.methods) {
            if (results.methods[mode].success && results.methods[mode].timeMs) {
                results.methods[mode].speedup = parseFloat((serialTime / results.methods[mode].timeMs).toFixed(2));
            }
        }
    }

    res.json(results);
});

app.listen(PORT, () => {
    console.log(`Server running at http://localhost:${PORT}`);
});
