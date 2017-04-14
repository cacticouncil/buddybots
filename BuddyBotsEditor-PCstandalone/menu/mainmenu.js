const {Menu} = require('electron')
const electron = require('electron')
const app = electron.app
var windows = require('./guide')
//const events = require('events')
//const emitter = new events()
//const ipc = require('electron').ipcRenderer;

const template = [
  {
      label:'File',
      submenu:[
          //{
          //    label:'New'
          //},
          {
              label:'Exit',
              role:'close'
          }
      ]
  },
  {
      role: 'help',
      submenu: [
        {
            label: 'Guide',
            click: function () { windows.create() }
            //click: function () { ipc.send('show') }
        },
        {
            label: 'Learn More',
            click () { require('electron').shell.openExternal('http://electron.atom.io') }
        }
      ]
  }
]

const menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)