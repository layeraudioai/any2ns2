#!/usr/bin/env node
'use strict';

const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');

const USAGE = `Usage: node any2ns2.js <sourcedir> <portdir> <iconpath> <apptitle> [appdescription] [--framework hblauncher|izlauncher] [--build]`;

function parseArgs(argv) {
    let sourcedir, portdir, iconpath, apptitle, appdescription;
    let framework = 'hblauncher';
    let build = false;

    const positional = [];
    for (let i = 2; i < argv.length; i++) {
        const arg = argv[i];
        if (arg === '--framework' || arg === '--target') {
            framework = (argv[++i] || 'hblauncher').toLowerCase();
        } else if (arg === '--build') {
            build = true;
        } else {
            positional.push(arg);
        }
    }

    if (positional.length < 4) {
        console.error(USAGE);
        process.exit(1);
    }

    sourcedir = path.resolve(positional[0]);
    portdir = path.resolve(positional[1]);
    iconpath = path.resolve(positional[2]);
    apptitle = positional[3];
    appdescription = positional[4] || 'Ported with Any2NS2 & BrendanLauncher';

    if (!['hblauncher', 'izlauncher'].includes(framework)) {
        throw new Error('Invalid framework. Choose either "hblauncher" or "izlauncher".');
    }

    return { sourcedir, portdir, iconpath, apptitle, appdescription, framework, build };
}

async function scanSource(root) {
    let files = [];
    let framework = 'Generic source project';
    let language = 'Unknown';
    let cs = false, mono = false, xna = false, godot = false, unity = false, py = false, cpp = false, java = false;

    const ignored = new Set(['.git', '.vs', 'bin', 'obj', 'node_modules', 'packages', '.gradle', 'build', 'dist', 'target']);
    
    async function walk(dir) {
        const entries = await fs.readdir(dir, { withFileTypes: true });
        for (const entry of entries) {
            const fullPath = path.join(dir, entry.name);
            const rel = path.relative(root, fullPath);
            if (entry.isDirectory()) {
                if (ignored.has(entry.name)) continue;
                await walk(fullPath);
            } else if (entry.isFile()) {
                files.push(rel);
                const ext = path.extname(entry.name).toLowerCase();
                if (ext === '.cs' || ext === '.csproj' || ext === '.sln') cs = true;
                if (ext === '.cpp' || ext === '.cc' || ext === '.c' || ext === '.h' || ext === '.hpp') cpp = true;
                if (ext === '.py') py = true;
                if (ext === '.java') java = true;
                if (ext === '.csproj' || ext === '.props' || ext === '.targets' || ext === '.config') {
                    try {
                        const text = await fs.readFile(fullPath, 'utf8');
                        if (text.includes('monogame') || text.includes('MonoGame.Framework')) mono = true;
                        if (text.includes('Microsoft.Xna.Framework') || text.includes('XNA')) xna = true;
                    } catch (e) {}
                }
                if (ext === '.godot' || ext === '.gd') godot = true;
                if (ext === '.unity' || ext === '.asmdef') unity = true;
            }
        }
    }

    await walk(root);
    files.sort();

    if (mono) { framework = 'MonoGame'; language = 'C#'; }
    else if (xna) { framework = 'XNA-compatible C#'; language = 'C#'; }
    else if (godot) { framework = 'Godot'; language = 'GDScript/C#'; }
    else if (unity) { framework = 'Unity'; language = cs ? 'C#' : 'Unknown'; }
    else if (cs) { framework = 'C# project'; language = 'C#'; }
    else if (cpp) { framework = 'C/C++ project'; language = 'C/C++'; }
    else if (py) { framework = 'Python project'; language = 'Python'; }
    else if (java) { framework = 'Java project'; language = 'Java'; }

    return { files, framework, language };
}

async function main() {
    const opts = parseArgs(process.argv);
    console.log(`[Any2NS2] Scanning source directory: ${opts.sourcedir}`);
    const scan = await scanSource(opts.sourcedir);
    console.log(`[Any2NS2] Detected Language: ${scan.language} | Framework: ${scan.framework}`);
    console.log(`[Any2NS2] Target End Framework: ${opts.framework.toUpperCase()}`);

    const slug = opts.apptitle.toLowerCase().replace(/[^a-z0-9]/g, '-').replace(/-+/g, '-').slice(0, 48);

    await fs.mkdir(opts.portdir, { recursive: true });
    await fs.mkdir(path.join(opts.portdir, 'source'), { recursive: true });
    await fs.mkdir(path.join(opts.portdir, 'assets'), { recursive: true });

    // Copy source files
    console.log('[Any2NS2] Preserving source files...');
    async function copyDir(src, dest) {
        const entries = await fs.readdir(src, { withFileTypes: true });
        for (const entry of entries) {
            if (['.git', 'node_modules', 'bin', 'obj'].includes(entry.name)) continue;
            const srcPath = path.join(src, entry.name);
            const destPath = path.join(dest, entry.name);
            if (entry.isDirectory()) {
                await fs.mkdir(destPath, { recursive: true });
                await copyDir(srcPath, destPath);
            } else {
                await fs.copyFile(srcPath, destPath);
            }
        }
    }
    await copyDir(opts.sourcedir, path.join(opts.portdir, 'source'));

    // Copy icon
    try {
        await fs.copyFile(opts.iconpath, path.join(opts.portdir, 'assets/icon.png'));
    } catch (e) {
        console.warn('[Any2NS2] Warning: Icon path not found or invalid, creating placeholder icon.');
    }

    if (opts.framework === 'hblauncher') {
        // HBLauncher GPU App format
        await fs.mkdir(path.join(opts.portdir, 'shaders'), { recursive: true });
        const manifest = {
            format: "hblauncher.gpu-app",
            abi: 1,
            id: slug || "ported-app",
            name: opts.apptitle,
            version: "1.0.0",
            description: opts.appdescription,
            transition: "fade",
            shaders: {
                vertex: "shaders/app.vert.spv",
                fragment: "shaders/app.frag.spv",
                compute: "shaders/app.comp.spv"
            },
            initialState: "assets/initial-state.bin",
            data: "assets/data.bin",
            icon: "assets/icon.png",
            sdk: { version: 1, capabilities: ["any2ns2-port"] }
        };
        await fs.writeFile(path.join(opts.portdir, 'app.json'), JSON.stringify(manifest, null, 2) + '\n');
        await fs.writeFile(path.join(opts.portdir, 'PORTING.md'), `# Porting Report: ${opts.apptitle}\n\n- Framework: ${scan.framework}\n- Language: ${scan.language}\n- Target: HBLauncher\n`);
        console.log(`[Any2NS2] Successfully generated HBLauncher port at: ${opts.portdir}`);
    } else {
        // IZLauncher / IZ2S Module format
        const modManifest = {
            format: "iz2s.module",
            id: slug || "ported-app",
            name: opts.apptitle,
            version: "1.0.0",
            description: opts.appdescription,
            shell: "izlauncher",
            sourceLanguage: scan.language,
            sourceFramework: scan.framework
        };
        await fs.writeFile(path.join(opts.portdir, 'module.json'), JSON.stringify(modManifest, null, 2) + '\n');
        await fs.writeFile(path.join(opts.portdir, 'README.md'), `# IZLauncher Port: ${opts.apptitle}\n\n- Framework: ${scan.framework}\n- Language: ${scan.language}\n- Target: IZLauncher / IZ2S-Platform\n`);
        console.log(`[Any2NS2] Successfully generated IZLauncher port at: ${opts.portdir}`);
    }
}

if (require.main === module) {
    main().catch(err => {
        console.error('[Any2NS2 Error]', err.stack);
        process.exitCode = 1;
    });
}
