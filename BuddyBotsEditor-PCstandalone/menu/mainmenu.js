const {Menu} = require('electron')
const electron = require('electron')
const app = electron.app

const template = [
  {
      label:'File',
      submenu:[
          {
              label:'New'
          },
          {
              label:'Load'
          },
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
            label: 'Guide'
        },
        {
            label:'About',
            role:'about'
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