# Droplet config editor
window.expressionContext = {
  prefix: 'a = '
}

window.dropletConfig = ace.edit 'droplet-config'
dropletConfig.setTheme 'ace/theme/chrome'
dropletConfig.getSession().setMode 'ace/mode/python'

# dropletConfig.setValue localStorage.getItem('config') ? '''
dropletConfig.setValue '''
  ({
    "mode": "python",
    "modeOptions": {
      "functions": {
        "myFunction": {
          "color": "purple",
          "dropdown": [['foo', 'bar'], ['baz', 'qux']]
        },
        'self': {
          'color': 'yellow'
        },
        'afiBotManager': {
          'color': 'green'
        },
        'botInput': {
          'color': 'red'
        },
      }
    },
    "palette": [
      {
        'name': 'Struct',
        'color': 'purple',
        'blocks': [
          {
            'block': "import sys"
          },
          {
            'block': "from statemachine import *"
          },
          {
            'block': "def Think(self, deltaTimeMS):\\n  return",
          },
          {
            'block':"botInput = aiInput_t()"
          },
          {
            'block':"botInput.moveFlag = aiMoveFlag_t.NULLMOVE"
          },
          {
            'block':"botInput.moveFlag = aiMoveFlag_t.RUN"
          },
          {
            'block': "def  Spawn(self, spawnDict):\\n  return",
          },
          {
            'block': "def  Restart(self):\\n  return",
          },
          {
            'block': "class YourBotName(afiBotBrain):\\n  return",
          },
          {
            'block': "botInput = aiInput_t()",
          },
          {
            'block': "self.spawnDict = spawnDict",
          },
          {
            'block': "self.enemyTeam = 0",
          },
          {
            'block': "self.myTeam = self.body.team",
          },
          {
            'block': "if self.myTeam == 0:\\n  self.enemyTeam = 1",
          },
          {
            'block': "return ''",
          },
          {
            'block': "self.part"
          },
          {
            'block': "self.enemyFlag"
          },
          {
            'block': "self.enemyTeam"
          },
          {
            'block': "self.ourFlag"
          },
          {
            'block': "botInput.part"
          },
        ]
      },
      {
        'name': 'Functions',
        'color': 'blue',
        'blocks': [
          {
            'block': "aiInput_t()"
          },
          {
            'block': "ConsolePrint(arg)"
          },
          {
            'block': "self.body.function"
          },
          {
            'block': "FindItem(item)"
          },
          {
            'block': "InView(entity)",
          },
          {
            'block': "MoveTo(pos,speed)",
          },
          {
            'block': "MoveToPosition(pos, rang)",
          },
          {
            'block': "MoveToEntity(entity)",
          },
          {
            'block': "MoveToPlayer(player)",
          },
          {
            'block': "Attack()"
          },
          {
            'block': "StopAttack()"
          },
          {
            'block': "Jump()"
          },
          {
            'block': "LookInDirection(direction)",
          },
          {
            'block': "LookAtPosition(pos)",
          },
          {
            'block': "MoveToNearest(item)",
          },
          {
            'block': "MoveToNearest(item)",
          },
          {
            'block': "PathToGoal()",
          },
          {
            'block': "ReachedPos(pos,rang)",
          },
          {
            'block': "SwitchWeapon(weaponName)"
          },
          {
            'block': "HasAmmo(weaponName)"
          },      
          {
            'block': "AmmoInClip()"
          },
          {
            'block': "FindNearbyPlayers()",
          },
          {
            'block': "FindItemsInView()",
          },
          {
            'block': "GetPosition()",
          },
          {
            'block': "NextWeapon()",
          },
          {
            'block': "UpdateAIMoveFlag(flag)"
          },
          {
            'block': "SaveLastTarget(entity)"
          },
          {
            'block': "GetLastTarget()"
          },
          {
            'block': "afiBotManager.function"
          },
          {
            'block': "GetFlagStatus(arg)"
          },
          {
            'block': "GetFlag(arg)"
          },
          {
            'block': "GetHealthByID(BotID)"
          },
          {
            'block': "GetTeamByID(BotID)"
          },
        ]
      },
      {
        'name': 'Control&Flags',
        'color': 'orange',
        'blocks': [
          {
            'block': "for i in range(0, 10):\\n  return",
          },
          {
            'block': "if a == b:\\n  return",
          },
          {
            'block': "while a < b:\\n  return",
          },
          {
            'block': "NULLMOVE",
          },
          {
            'block': "CROUCH",
          },
          {
            'block': "JUMP",
          },
          {
            'block': "WALK",
          },
          {
            'block': "RUN",
          },
          {
            'block': "flagStatus_t.FLAGSTATUS_INBASE",
          },
          {
            'block': "aiMoveFlag_t.NULLMOVE",
          },
        ]
      },
      {
        'name': 'Variables',
        'color': 'green',
        'blocks': [
          {
            'block': 'a = 1'
          },
          {
            'block': "weapon_flashlight"
          },
          {
            'block': "weapon_pistol"
          },
          {
            'block': "weapon_machinegun_mp"
          },
          {
            'block': "weapon_shotgun"
          },
          {
            'block': "weapon_rocketlauncher"
          },
          {
            'block': "weapon_handgrenade"
          },
          {
            'block': "weapon_chainsaw"
          },
          {
            'block': "weapon_fists"
          },
          {
            'block': "item_medkit"
          },
          {
            'block': "item_medkit_small"
          },
          {
            'block': "body"
          },
          {
            'block': "enemyFlag"
          },
          {
            'block': "ourFlag"
          },
          {
            'block': "spawnDict"
          },
          {
            'block': "enemyTeam"
          },
          {
            'block': "myTeam"
          },
          {
            'block': "moveFlag"
          },
        ]
      },
      {
        'name': 'Logic',
        'color': 'teal',
        'blocks': [
          {
            'block': 'a == b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a < b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a > b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a and b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a or b',
            'wrapperContext': expressionContext
          }
        ]
      },
      {
        'name': 'Math',
        'color': 'red',
        'blocks': [
          {
            'block': 'a + b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a - b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a * b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a / b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a % b',
            'wrapperContext': expressionContext
          },
          {
            'block': 'a ** b',
            'wrapperContext': expressionContext
          }
        ]
      }
    ]
  })
'''

# Droplet itself
createEditor = (options) ->
  $('#droplet-editor').html ''
  editor = new droplet.Editor $('#droplet-editor')[0], options

  editor.setEditorState localStorage.getItem('blocks') is 'yes'
  editor.aceEditor.getSession().setUseWrapMode true

  # Initialize to starting text
  localStorage.clear()
  editor.setValue localStorage.getItem('text') ? ''
  #basedata = "import sys\nsys.path.append(\"./d3xp/botPaks/Sample.pk4\")\nfrom statemachine import *\nclass Sample(afiBotBrain):\n def Think(self , deltaTimeMS):\n  return botInput\n def Spawn(self,spawnDict):\n  return\n def Restart(self):\n  return"
  #localStorage.setItem('text',basedata)
  #editor.setValue localStorage.getItem('text')

  editor.on 'change', ->
    localStorage.setItem 'text', editor.getValue()

  window.editor = editor

createEditor eval dropletConfig.getValue()

$('#toggle').on 'click', ->
  editor.toggleBlocks()
  localStorage.setItem 'blocks', (if editor.currentlyUsingBlocks then 'yes' else 'no')

$('#export').on 'click', ->
  content = localStorage.getItem('text')
  check1 = "import sys\nfrom afiBotBrain import *\nfrom afiBotManager import *\nfrom afiBotPlayer import *\nfrom idPlayer import *\nfrom idActor import *\nfrom idEntity import *\nfrom idVec3 import *\nclass " + document.getElementById('ainame').value + "(afiBotBrain):\n def Think(self , deltaTimeMS):\n  botInput = aiInput_t()"
  check2 = "return botInput\n def Spawn(self,spawnDict):"
  check3 = "  return\n def Restart(self):"
  if content.indexOf(check1)>=0 && content.indexOf(check2)>=0 && content.indexOf(check3)>=0
     document.getElementById('check').value = "Correct"
     console.log(content);
  else
     document.getElementById('check').value = "Incorrect"
     alert("Bot structure is not correct Please check guide")
     #localStorage.clear()
     #localStorage.setItem('text',"Incorrect bot structure")

$('#showexample').on 'click', ->
  localStorage.clear()
  document.getElementById('username').value = 'Sample'
  document.getElementById('ainame').value = 'Sample'
  data = "import sys\nfrom afiBotBrain import *\nfrom afiBotManager import *\nfrom afiBotPlayer import *\nfrom idPlayer import *\nfrom idActor import *\nfrom idEntity import *\nfrom idVec3 import *\nclass Sample(afiBotBrain):\n def Think(self , deltaTimeMS):\n  botInput = aiInput_t()\n  botInput.moveFlag = aiMoveFlag_t.NULLMOVE\n  if afiBotManager.GetFlagStatus(self.enemyTeam) == flagStatus_t.FLAGSTATUS_INBASE:\n    self.enemyFlag = afiBotManager.GetFlag(self.enemyTeam)\n    self.body.MoveToPosition(self.enemyFlag.GetPosition(),8)\n    self.body.LookAtPosition(self.enemyFlag.GetPosition())\n  elif afiBotManager.GetFlagStatus(self.enemyTeam) == flagStatus_t.FLAGSTATUS_TAKEN:\n    self.ourFlag = afiBotManager.GetFlag(self.myTeam)\n    self.body.MoveToPosition(self.ourFlag.GetPosition(),8)\n    self.body.LookAtPosition(self.ourFlag.GetPosition())\n  return botInput\n def Spawn(self,spawnDict):\n  self.spawnDict = spawnDict\n  self.enemyTeam = 0\n  self.myTeam = self.body.team\n  if self.myTeam == 0:\n    self.enemyTeam = 1\n  return\n def Restart(self):\n  return"
  localStorage.setItem('text',data)
  editor.setValue localStorage.getItem('text')

$('#quickstart').on 'click', ->
  localStorage.clear()
  if document.getElementById('username').value && document.getElementById('ainame').value
    if document.getElementById('ainame').value == "is" || document.getElementById('ainame').value == "if" || document.getElementById('ainame').value == "of" || document.getElementById('ainame').value == "as" || document.getElementById('ainame').value == "return" || document.getElementById('ainame').value == "import" || document.getElementById('ainame').value == "def" || document.getElementById('ainame').value == "from" || document.getElementById('ainame').value == "class" || document.getElementById('ainame').value == "for" || document.getElementById('ainame').value == "in" || document.getElementById('ainame').value == "while" || document.getElementById('ainame').value == "range"
       alert("Ainame is invalid")
    else
       data = "import sys\nfrom afiBotBrain import *\nfrom afiBotManager import *\nfrom afiBotPlayer import *\nfrom idPlayer import *\nfrom idActor import *\nfrom idEntity import *\nfrom idVec3 import *\nclass " + document.getElementById('ainame').value + "(afiBotBrain):\n def Think(self , deltaTimeMS):\n  botInput = aiInput_t()\n  return botInput\n def Spawn(self,spawnDict):\n  self.spawnDict = spawnDict\n  self.enemyTeam = 0\n  self.myTeam = self.body.team\n  if self.myTeam == 0:\n    self.enemyTeam = 1\n  return\n def Restart(self):\n  return"
       localStorage.setItem('text',data)
       editor.setValue localStorage.getItem('text')
  else
    alert("Need both Author name and bot name")#data = "import sys\nsys.path.append(\"./d3xp/botPaks/" + "SampleBots" + ".pk4\")\nfrom statemachine import *\nclass " + "SampleBots" + "(afiBotBrain):\n def Think(self , deltaTimeMS):\n  return botInput\n def Spawn(self,spawnDict):\n  return\n def Restart(self):\n  return"

$('#new').on 'click', ->
  localStorage.clear()
  document.getElementById('username').value = ''
  document.getElementById('ainame').value = ''
  localStorage.setItem('text',"")
  editor.setValue localStorage.getItem('text')

$('#restart').on 'click', ->
  if localStorage.getItem('text') != ""
    localStorage.clear()
    if document.getElementById('username').value && document.getElementById('ainame').value
        if document.getElementById('ainame').value == "is" || document.getElementById('ainame').value == "if" || document.getElementById('ainame').value == "of" || document.getElementById('ainame').value == "as" || document.getElementById('ainame').value == "return" || document.getElementById('ainame').value == "import" || document.getElementById('ainame').value == "def" || document.getElementById('ainame').value == "from" || document.getElementById('ainame').value == "class" || document.getElementById('ainame').value == "for" || document.getElementById('ainame').value == "in" || document.getElementById('ainame').value == "while" || document.getElementById('ainame').value == "range"
            alert("Ainame is invalid")
        else
            data = "import sys\nfrom afiBotBrain import *\nfrom afiBotManager import *\nfrom afiBotPlayer import *\nfrom idPlayer import *\nfrom idActor import *\nfrom idEntity import *\nfrom idVec3 import *\nclass " + document.getElementById('ainame').value + "(afiBotBrain):\n def Think(self , deltaTimeMS):\n  botInput = aiInput_t()\n  return botInput\n def Spawn(self,spawnDict):\n  self.spawnDict = spawnDict\n  self.enemyTeam = 0\n  self.myTeam = self.body.team\n  if self.myTeam == 0:\n    self.enemyTeam = 1\n  return\n def Restart(self):\n  return"
            localStorage.setItem('text',data)
            editor.setValue localStorage.getItem('text')
    else
        alert("Need both Author name and bot name")

$('#clearteam').on 'click', ->
  document.getElementById('teamname').value = ''
  document.getElementById('leader').value = ''
  document.getElementById('veteran').value = ''
  document.getElementById('recruit').value = ''
  document.getElementById('recruit2').value = ''

# Stuff for testing convenience
$('#update').on 'click', ->
  localStorage.setItem 'config', dropletConfig.getValue()
  createEditor eval dropletConfig.getValue()

configCurrentlyOut = localStorage.getItem('configOut') is 'yes'

updateConfigDrawerState = ->
  if configCurrentlyOut
    $('#left-panel').css 'left', '0px'
    $('#right-panel').css 'left', '225px'
  else
    $('#left-panel').css 'left', '-200px'
    $('#right-panel').css 'left', '25px'

  editor.resize()

  localStorage.setItem 'configOut', (if configCurrentlyOut then 'yes' else 'no')

$('#close').on 'click', ->
  configCurrentlyOut = not configCurrentlyOut
  updateConfigDrawerState()

updateConfigDrawerState()