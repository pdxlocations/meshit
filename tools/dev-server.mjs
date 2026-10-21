import http from 'node:http';
import {readFile} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';
const sim=spawn('./output/simulator',[],{stdio:['pipe','pipe','inherit']}),pending=[];
createInterface({input:sim.stdout}).on('line',line=>{pending.shift()?.(line);});
const mime={'.html':'text/html','.js':'text/javascript','.css':'text/css'};
const server=http.createServer(async(req,res)=>{
  try{
    if(req.method==='POST'&&req.url==='/api/rpc'){
      let body='';for await(const chunk of req){body+=chunk;if(body.length>4096){res.writeHead(413).end();return;}}JSON.parse(body);
      const reply=new Promise(resolve=>pending.push(resolve));sim.stdin.write(body+'\n');res.writeHead(200,{'Content-Type':'application/json'}).end(await reply);return;
    }
    const file=req.url==='/'?'index.html':req.url?.slice(1);if(!['index.html','app.js','transport.js','style.css'].includes(file)){res.writeHead(404).end();return;}
    res.writeHead(200,{'Content-Type':mime['.'+file.split('.').pop()],'Cache-Control':'no-store'}).end(await readFile('web/'+file));
  }catch(e){res.writeHead(400,{'Content-Type':'application/json'}).end(JSON.stringify({error:e.message}));}
});
const port=Number(process.env.PORT||8787);server.listen(port,'127.0.0.1',()=>console.log(`Meshit SIMULATOR: http://127.0.0.1:${port} (no radio traffic; rebroadcasts simulated)`));
function close(){sim.kill();server.close();}process.on('SIGINT',close);process.on('SIGTERM',close);
