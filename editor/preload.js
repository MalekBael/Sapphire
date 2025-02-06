const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    getJsonData: (fileName) => ipcRenderer.invoke('get-json-data', fileName),
    saveScript: (content) => ipcRenderer.invoke('save-script', content)
});
