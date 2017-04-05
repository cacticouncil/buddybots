const {Menu} = require('electron')
const electron = require('electron')
const app = electron.app

const template = [
  {
      label:'File',
      submenu:[
          {
              label:'Exit',
              role:'close'
          }
      ]
  }
]

const menu2 = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu2)