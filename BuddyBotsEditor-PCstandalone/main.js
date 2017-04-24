const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow
const path = require('path')
const url = require('url')
//const ipcMain = require('electron').ipcMain
//var Gwindow = require('./menu/guide.js')
//const ipcRenderer = require('electron').ipcRenderer

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
//let win

function createWindow () {
    // Create the browser window.
    win = new BrowserWindow({width: 1440, height: 780})

    // and load the index.html of the app.
    win.loadURL(url.format({
        pathname: path.join(__dirname, 'index.html'),
        protocol: 'file:',
        slashes: true
    }))

    // Emitted when the window is closed.
    win.on('closed', () => {
        win = null
    })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
//app.on('ready', createWindow)
app.on('ready', function () {
    var mainWindow = new BrowserWindow({
        width: 1440,
        height: 780
    })

    mainWindow.loadURL('file://' + __dirname + '/index.html')

    //mainWindow.openDevTools();

    //var guideWindow = new BrowserWindow({
    //    width: 800,
    //    height: 400,
    //    show:false
    //})

    //guideWindow.loadURL('file://' + __dirname + '/guide.html')

    //ipcMain.on('show', function () {
    //    guideWindow.show()
    //})

    //ipcMain.on('hide', function () {
    //    guideWindow.hide()
    //})

    require('./menu/mainmenu')

    mainWindow.on('closed', () => {
        mainWindow = null
    })
})

// Quit when all windows are closed.
app.on('window-all-closed', () => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
     //Gwindow.closeGuide()
    if (process.platform !== 'darwin') {
        app.quit()
    }
})

app.on('activate', () => {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (win === null) {
        createWindow()
    }
})
