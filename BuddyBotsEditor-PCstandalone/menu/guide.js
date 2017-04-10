var electron = require('electron')
var BrowserWindow = electron.BrowserWindow
var ipcMain = electron.ipcMain

ipcMain.on('create-window', create)

var ifOpen = true

function create() {
    if (ifOpen) {
        var guideWindow = new BrowserWindow({
            width: 800,
            height: 800,
            //frame: false
        })

        guideWindow.loadURL('file://' + __dirname + '/guide.html')

        guideWindow.setMenu(null)
        //require('./submenu')
        ifOpen = false


        guideWindow.on('closed', function () {
            ifOpen = true
            guideWindow = null
        })
    }
}

function closeGuide() {
    ifOpen = true
    guideWindow = null
    //if (process.platform !== 'darwin') {
    //    app.quit()
    //}
}

module.exports = { create, closeGuide }