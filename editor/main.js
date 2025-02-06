const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');

function createWindow() {
    // Create the browser window.
    const mainWindow = new BrowserWindow({
        width: 1400,
        height: 1200,
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: path.join(__dirname, 'preload.js')
        }
    });

    // Load the index.html file
    mainWindow.loadFile('index.html');

    // Open DevTools for debugging (you can remove this later)
    // mainWindow.webContents.openDevTools();
}

app.whenReady().then(() => {
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

// IPC Handlers
ipcMain.handle('get-json-data', async (event, fileName) => {
    try {
        const filePath = path.join(__dirname, fileName);
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (error) {
        console.error(`Error reading ${fileName}:`, error);
        return [];
    }
});

ipcMain.handle('save-script', async (event, content) => {
    try {
        const outputDir = path.join(__dirname, 'output');
        if (!fs.existsSync(outputDir)) {
            fs.mkdirSync(outputDir);
        }
        const outputPath = path.join(outputDir, 'BnpcSecondHoplomachus.cpp');
        fs.writeFileSync(outputPath, content);
        return true;
    } catch (error) {
        console.error('Error saving script:', error);
        return false;
    }
});
