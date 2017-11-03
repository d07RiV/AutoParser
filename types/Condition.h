#pragma once
#include "snotypes.h"
#include "serialize.h"
#include "snocommon.h"

#pragma pack(push, 1)

declstruct(Condition::Type) {
  declstruct(LoreSubcondition) {
    Sno<Lore> x00_LoreSno;
    int x04;
    void dumpfunc() {
      dumpval(x00_LoreSno, x04);
    }
  };

  declstruct(QuestSubcondition) {
    Sno<Quest> x00_QuestSno;
    int x04;
    int x08;
    int x0C;
    void dumpfunc() {
      dumpval(x00_QuestSno, x04, x08, x0C);
    }
  };

  declstruct(ItemSubcondition) {
    GameBalanceId x00_ItemsGameBalanceId;
    int x04;
    int x08;
    int x0C;
    void dumpfunc() {
      dumpval(x00_ItemsGameBalanceId, x04, x08, x0C);
    }
  };

  declstruct(FollowerSubcondition) {
    int x00_Enum; // - None - = 0, Templar = 1, Scoundrel = 2, Enchantress = 3
    int x04;
    void dumpfunc() {
      dumpval(x00_Enum, x04);
    }
  };

  declstruct(LabelSubcondition) {
    GameBalanceId x00_LabelsGameBalanceId;
    int x04;
    void dumpfunc() {
      dumpval(x00_LabelsGameBalanceId, x04);
    }
  };

  declstruct(SkillSubcondition) {
    Sno<Power> x00_PowerSno;
    int x04;
    int x08;
    void dumpfunc() {
      dumpval(x00_PowerSno, x04, x08);
    }
  };

  declstruct(MonsterSubcondition) {
    Sno<Actor> x00_ActorSno;
    void dumpfunc() {
      dumpval(x00_ActorSno);
    }
  };

  declstruct(GameFlagSubcondition) {
    char x00_Text[128];
    void dumpfunc() {
      dumpval(x00_Text);
    }
  };

  declstruct(PlayerFlagSubcondition) {
    char x00_Text[128];
    void dumpfunc() {
      dumpval(x00_Text);
    }
  };

  declstruct(BuffSubcondition) {
    Sno<Power> x00_PowerSno;
    int x04;
    int x08;
    void dumpfunc() {
      dumpval(x00_PowerSno, x04, x08);
    }
  };

  SnoHeader x000_Header;
  int x00C;
  int x010;
  int x014_int[6];
  int x02C;
  int x030;
  int x034;
  int x038;
  int x03C;
  LoreSubcondition x040_LoreSubconditions[3];
  QuestSubcondition x058_QuestSubconditions[3];
  int x088;
  int x08C;
  int x090;
  ItemSubcondition x094_ItemSubconditions[3];
  int x0C4;
  int x0C8;
  int x0CC;
  int x0D0;
  int x0D4;
  int x0D8;
  int x0DC;
  int x0E0;
  uint32 x0E4_;
  int x0E8;
  int x0EC;
  int x0F0;
  int x0F4;
  Sno<Worlds> x0F8_WorldsSno;
  Sno<LevelArea> x0FC_LevelAreaSno;
  Sno<QuestRange> x100_QuestRangeSno;
  FollowerSubcondition x104_FollowerSubcondition;
  LabelSubcondition x10C_LabelSubconditions[3];
  SkillSubcondition x124_SkillSubconditions[3];
  int x148;
  int x14C;
  int x150;
  int x154;
  int x158;
  int x15C;
  int x160;
  int x164;
  int x168;
  MonsterSubcondition x16C_MonsterSubconditions[15];
  GameFlagSubcondition x1A8_GameFlagSubconditions[3];
  PlayerFlagSubcondition x328_PlayerFlagSubconditions[3];
  BuffSubcondition x4A8_BuffSubconditions[3];
  int x4CC;
  int x4D0;
  int x4D4;
  void dumpfunc() {
    dumpval(x000_Header, x00C, x010, x014_int, x02C, x030);
    dumpval(x034, x038, x03C, x040_LoreSubconditions, x058_QuestSubconditions, x088);
    dumpval(x08C, x090, x094_ItemSubconditions, x0C4, x0C8, x0CC);
    dumpval(x0D0, x0D4, x0D8, x0DC, x0E0);
    dumpval(x0E8, x0EC, x0F0, x0F4, x0F8_WorldsSno);
    dumpval(x0FC_LevelAreaSno, x100_QuestRangeSno, x104_FollowerSubcondition, x10C_LabelSubconditions, x124_SkillSubconditions, x148);
    dumpval(x14C, x150, x154, x158, x15C, x160);
    dumpval(x164, x168, x16C_MonsterSubconditions, x1A8_GameFlagSubconditions, x328_PlayerFlagSubconditions, x4A8_BuffSubconditions);
    dumpval(x4CC, x4D0, x4D4);
  }
};
structsize(Condition::Type, 0x4D8);

#pragma pack(pop)
