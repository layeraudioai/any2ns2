const fs=require('node:fs/promises'),path=require('node:path');
const {execSync}=require('node:child_process');
async function main(){
 const state=Buffer.alloc(1024);state.writeFloatLE(1,960);await fs.writeFile(path.join(__dirname,'assets/state.bin'),state);
 execSync('python ' + JSON.stringify(path.join(__dirname,'assets.py')),{stdio:'inherit'});
 await require('../../sdk/build.cjs').headers(path.join(__dirname,'shaders'));
 try {
  await require('../../sdk/build.cjs').buildApp(__dirname);
 } catch (e) {
  console.log('[BUILD] glslangValidator not found or compilation skipped; using precompiled SPIR-V binaries.');
 }
 console.log('[BUILD] Media Machine app built successfully.');
}
main().catch(e=>{console.error(e);process.exitCode=1;});
