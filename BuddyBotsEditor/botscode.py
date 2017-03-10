import sys
sys.path.append("./d3xp/botPaks/SampleBot.pk4")
from statemachine import *
class SampleBot(afiBotBrain):
	def Think(self , deltaTimeMS):
		botInput = aiInput_t()
		if afiBotManager.GetFlagStatus(self.enemyTeam) == flagStatus_t.FLAGSTATUS_INBASE:
		  self.enemyFlag = afiBotManager.GetFlag(self.enemyTeam)
		  self.body.MoveToPosition(self.enemyFlag.GetPosition(), 8)
		  self.body.LookAtPosition(self.enemyFlag.GetPosition())
		elif afiBotManager.GetFlagStatus(self.enemyTeam) == flagStatus_t.FLAGSTATUS_TAKEN:
			self.ourFlag = afiBotManager.GetFlag(self.myTeam)
			self.body.MoveToPosition(self.ourFlag.GetPosition(),8)
			self.body.LookAtPosition(self.ourFlag.GetPosition())
		return botInput
	def Spawn(self,spawnDict):
		self.spawnDict = spawnDict
		self.enemyTeam = 0
		self.myTeam = self.body.team
		afiBotManager.ConsolePrint("Grabbed Our Flag")
		str = "BoitanoBot Team = {}\n".format(self.myTeam)
		afiBotManager.ConsolePrint(str)
		if self.myTeam == 0:
			self.enemyTeam = 1
		return
	def Restart(self):
		return