var electron = require('electron')
var BrowserWindow = electron.BrowserWindow
var ipcMain = electron.ipcMain

ipcMain.on('create-window', create)
ipcMain.on('close-window', closeGuide)

var ifOpen = true

var guideWindow = null

function create() {
    if (ifOpen) {
        guideWindow = new BrowserWindow({
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
    guideWindow.close()
    //if (process.platform !== 'darwin') {
    //    app.quit()
    //}
}

module.exports = { create,closeGuide }