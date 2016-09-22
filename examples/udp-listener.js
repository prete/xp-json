const dgram = require('dgram');
const client = dgram.createSocket('udp4');

client.on('error', (err) => {
  console.log(`error:\n${err.stack}`);
  client.close();
});

client.on('message', (msg, rinfo) => {
    console.log(JSON.parse(msg.toString('utf8')))
    //console.log(`got: ${msg} from ${rinfo.address}:${rinfo.port}`);
});

client.on('listening', () => {
  var address = client.address();
  console.log(`listening ${address.address}:${address.port}`);
});

client.bind(51000);