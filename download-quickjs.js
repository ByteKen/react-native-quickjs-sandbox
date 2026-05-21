const fs = require('fs');
const path = require('path');
const https = require('https');

const targetDir = path.join(__dirname, 'cpp', 'quickjs');
if (!fs.existsSync(targetDir)) {
  fs.mkdirSync(targetDir, { recursive: true });
}

// We use the original bellard/quickjs repository for stable core files.
const baseUrl = 'https://raw.githubusercontent.com/bellard/quickjs/master/';
const files = [
  'quickjs.c',
  'quickjs.h',
  'quickjs-atom.h',
  'quickjs-opcode.h',
  'cutils.c',
  'cutils.h',
  'dtoa.c',
  'dtoa.h',
  'libregexp.c',
  'libregexp.h',
  'libregexp-opcode.h',
  'list.h',
  'libunicode.c',
  'libunicode.h',
  'libunicode-table.h',
];

function download(file) {
  return new Promise((resolve, reject) => {
    const url = baseUrl + file;
    const dest = path.join(targetDir, file);
    const fileStream = fs.createWriteStream(dest);

    https.get(url, (response) => {
      if (response.statusCode !== 200) {
        reject(new Error(`Failed to download ${file}: Status code ${response.statusCode}`));
        return;
      }
      response.pipe(fileStream);
      fileStream.on('finish', () => {
        fileStream.close();
        console.log(`Downloaded ${file}`);
        resolve();
      });
    }).on('error', (err) => {
      fs.unlink(dest, () => {});
      reject(err);
    });
  });
}

async function run() {
  console.log('Downloading QuickJS engine files...');
  for (const file of files) {
    try {
      await download(file);
    } catch (e) {
      console.error(`Error downloading ${file}:`, e.message);
      process.exit(1);
    }
  }
  console.log('All QuickJS engine files downloaded successfully!');
}

run();
