var electron = require('electron')
var BrowserWindow = electron.BrowserWindow
var ipcMain = electron.ipcMain

ipcMain.on('create-window', create)

function create() {
    var guideWindow = new BrowserWindow({
        width: 800,
        height: 800,
        frame: false
    })

    guideWindow.loadURL('file://' + __dirname + '/guide.html')

    //require('./submenu')

    guideWindow.on('closed', function () {
        guideWindow = null
    })
}

module.exports = { create }