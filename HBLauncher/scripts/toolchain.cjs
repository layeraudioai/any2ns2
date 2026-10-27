const fs=require('node:fs'),path=require('node:path');
function tool(environment,name){
 const executable=name+(process.platform==='win32'?'.exe':'');
 const local=path.resolve(__dirname,'../tools',executable);
 return process.env[environment]||(fs.existsSync(local)?local:executable);
}
module.exports={tool};
