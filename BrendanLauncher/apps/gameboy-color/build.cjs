// Build the shader-only Game Boy app. Pass a ROM path, or use --placeholder
// to create a safe idle ROM for launcher/manifest validation only.
const fs = require('node:fs/promises');
const path = require('node:path');
const zlib = require('node:zlib');
const { buildApp, compile } = require('../../sdk/build.cjs');

const root = __dirname;
const MEMORY_BYTES = 18 * 1024 * 1024;
const ROM_LIMIT = 8 * 1024 * 1024;

async function main() {
  const arg = process.argv[2];
  if (!arg) throw new Error('Usage: node build.cjs <game.gb|game.gbc> (or --placeholder for validation only)');

  let rom;
  if (arg === '--placeholder') {
    rom = Buffer.alloc(0x8000);
    // Post-boot CPU starts at 0x0100. JR -2 safely idles forever.
    rom[0x100] = 0x18;
    rom[0x101] = 0xfe;
  } else {
    rom = await fs.readFile(path.resolve(arg));
    if (rom.length < 0x150 || rom.length > ROM_LIMIT)
      throw new Error(`ROM must be between 336 bytes and ${ROM_LIMIT} bytes`);
  }

  const assets = path.join(root, 'assets');
  await fs.mkdir(assets, { recursive: true });
  const state = Buffer.alloc(1024);
  state.writeFloatLE(1, 960);
  await fs.writeFile(path.join(assets, 'state.bin'), state);
  await fs.writeFile(path.join(assets, 'text.bin'), Buffer.alloc(1024 * 2048));

  const memory = Buffer.alloc(MEMORY_BYTES);
  rom.copy(memory, 0);
  await fs.writeFile(path.join(assets, 'memory.bin'), zlib.deflateSync(memory, { level: 9 }));
  await fs.copyFile(path.join(root, '..', 'hello-world', 'assets', 'icon.png'), path.join(assets, 'icon.png'));

  await buildApp(root);
  for (const stage of ['begin', 'after']) {
    const source = path.join(root, 'shaders', `app.${stage}`);
    // glslang auto-detects shader type from the file extension. These source
    // files are named .begin/.after, so compile temporary .comp copies.
    const stagedSource = `${source}.comp`;
    await fs.copyFile(source, stagedSource);
    try {
      compile(stagedSource, path.join(root, `app.${stage}.spv`));
    } finally {
      await fs.rm(stagedSource, { force: true });
    }
  }
  console.log(arg === '--placeholder'
    ? 'Built validation-only placeholder. Rebuild with your .gb/.gbc ROM to play.'
    : `Built shader app with ${path.basename(arg)} (${rom.length} bytes).`);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
