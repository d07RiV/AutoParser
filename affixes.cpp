#include "affixes.h"
#include "description.h"
#include "strings.h"
#include "powertag.h"
#include "regexp.h"
#include <algorithm>

int fixAttrId(int id, bool reverse) {
  static bool fixLoaded = false;
  static int fix_forward[2048];
  static int fix_reverse[2048];
  if (!fixLoaded) {
    fixLoaded = true;

    memset(fix_forward, -1, sizeof fix_forward);
    memset(fix_reverse, -1, sizeof fix_reverse);
    auto const& fix = GameAffixes::rawData()["AffixFix"];
    std::map<istring, int> attrList;
    for (auto const& kv : fix.getMap()) {
      int index = std::stoi(kv.first);
      size_t spos = kv.second.getString().find('/');
      if (spos != std::string::npos) {
        PowerTag* tag = PowerTags::get(kv.second.getString().substr(0, spos));
        if (tag) {
          auto const& formula = tag->formula(kv.second.getString().substr(spos + 1));
          uint32 const* begin = formula.data();
          uint32 const* end = begin + formula.size();
          uint32 x, y/*, z, w*/;
          while (begin < end) {
            switch (*begin++) {
            case 1:
            case 6:
              ++begin;
              break;
            case 5:
              x = *begin++;
              y = *begin++;
              /*z = **/begin++;
              /*w = **/begin++;
              if (x == 0) {
                fix_forward[y] = index;
                fix_reverse[index] = y;
                begin = end;
              }
              break;
            case 0:
              begin = end;
              break;
            }
          }
        }
      } else {
        attrList[kv.second.getString()] = index;
      }
    }
    SnoFile<GameBalance> gmb("1xx_AffixList");
    for (auto& affix : gmb->x078_AffixTable) {
      if (attrList.count(affix.x000_Text)) {
        int lhs = attrList[affix.x000_Text];
        int rhs = affix.x260_AttributeSpecifiers[0].x00_Type;
        fix_forward[rhs] = lhs;
        fix_reverse[lhs] = rhs;
      }
    }

    for (int i = 1; i < 2048; ++i) {
      if (fix_forward[i] < 0) fix_forward[i] = fix_forward[i - 1] + 1;
      if (fix_reverse[i] < 0) fix_reverse[i] = fix_reverse[i - 1] + 1;
    }
  }
  return (reverse ? fix_reverse : fix_forward)[id];
}

bool GameAffixes::isSecondary(uint32 attr) {
  return instance().affixData_.secondary.count(fixAttrId(attr)) != 0;
}

AttributeSpecifier::AttributeSpecifier(GameBalance::Type::AttributeSpecifier const& attr, AttributeMap const& params)
  : type(attr.x00_Type)
  , param(attr.x04_Param)
{
  if (type != -1U) {
    value = ExecFormula(
      reinterpret_cast<uint32 const*>(attr.x08_Data.begin()),
      reinterpret_cast<uint32 const*>(attr.x08_Data.end()),
      params);
  }
}

struct GameAffixes::GameAffix {
  std::set<uint32> itemTypes;
  AffixValue value;
};

GameAffixes::GameAffixes() {
  defaultMap_.emplace("Level", 70.0);
  defaultMap_.emplace("Casting_Speed_Total", 1.0);
  defaultMap_.emplace("Attacks_Per_Second_Total", 1.0);
  defaultMap_.emplace("Effective_Level", 70.0);
  defaultMap_.emplace("iLevel", 72.0);

  SnoFile<GameBalance> affixList("1xx_AffixList");
  for (auto& fx : affixList->x078_AffixTable) {
    uint32 id = HashNameLower(fx.x000_Text);
    GameAffix& affix = affixes_[id];
    for (auto id : fx.x17C_GameBalanceIds) {
      if (id != -1U) affix.itemTypes.insert(id);
    }
    for (auto id : fx.x194_GameBalanceIds) {
      if (id != -1U) affix.itemTypes.insert(id);
    }
    for (auto id : fx.x1F4_GameBalanceIds) {
      if (id != -1U) affix.itemTypes.insert(id);
    }
    for (size_t i = 0; i < AffixValue::MaxAttributes; ++i) {
      affix.value.attributes[i] = AttributeSpecifier(fx.x260_AttributeSpecifiers[i], defaultMap_);
    }
    std::sort(affix.value.attributes, affix.value.attributes + AffixValue::MaxAttributes, AttributeSpecifier::less);
    if (fx.x16C_AffixGroupGameBalanceId != -1U) {
      groups_[fx.x16C_AffixGroupGameBalanceId].push_back(&affix);
    }
    if (fx.x170_AffixGroupGameBalanceId != -1U) {
      groups_[fx.x170_AffixGroupGameBalanceId].push_back(&affix);
    }
  }

  SnoFile<GameBalance> affixListRecipes("AffixList");
  for (auto& fx : affixListRecipes->x078_AffixTable) {
    uint32 id = HashNameLower(fx.x000_Text);
    GameAffix& affix = affixes_[id];
    for (size_t i = 0; i < AffixValue::MaxAttributes; ++i) {
      affix.value.attributes[i] = AttributeSpecifier(fx.x260_AttributeSpecifiers[i], defaultMap_);
    }
    std::sort(affix.value.attributes, affix.value.attributes + AffixValue::MaxAttributes, AttributeSpecifier::less);
  }

  SnoFile<GameBalance> itemTypes("ItemTypes");
  for (auto& type : itemTypes->x018_ItemTypes) {
    if (type.x108_ItemTypesGameBalanceId != -1U) {
      itemTypeParent_[HashNameLower(type.x000_Text)] = type.x108_ItemTypesGameBalanceId;
    }
  }

  json::parse(File(path::work() / "affixes.js"), affixData_.raw);
  for (auto& kv : affixData_.raw["Affixes"].getMap()) {
    affixData_.types[atoi(kv.first.c_str())] = kv.second.getString();
  }
  for (auto& kv : affixData_.raw["Secondary"].getMap()) {
    affixData_.secondary.insert(atoi(kv.first.c_str()));
  }

  itemTypes_[HashNameLower("TemplarSpecial")] = "templarrelic";
  itemTypes_[HashNameLower("ScoundrelSpecial")] = "scoundreltoken";
  itemTypes_[HashNameLower("EnchantressSpecial")] = "enchantressfocus";
  itemTypes_[HashNameLower("Helm")] = "helm";
  itemTypes_[HashNameLower("WizardHat")] = "wizardhat";
  itemTypes_[HashNameLower("VoodooMask")] = "voodoomask";
  itemTypes_[HashNameLower("SpiritStone_Monk")] = "spiritstone";
  itemTypes_[HashNameLower("Shoulders")] = "shoulders";
  itemTypes_[HashNameLower("Amulet")] = "amulet";
  itemTypes_[HashNameLower("ChestArmor")] = "chestarmor";
  itemTypes_[HashNameLower("Cloak")] = "cloak";
  itemTypes_[HashNameLower("Belt")] = "belt";
  itemTypes_[HashNameLower("Belt_Barbarian")] = "mightybelt";
  itemTypes_[HashNameLower("Gloves")] = "gloves";
  itemTypes_[HashNameLower("Bracers")] = "bracers";
  itemTypes_[HashNameLower("Legs")] = "pants";
  itemTypes_[HashNameLower("Boots")] = "boots";
  itemTypes_[HashNameLower("Ring")] = "ring";
  itemTypes_[HashNameLower("Axe")] = "axe";
  itemTypes_[HashNameLower("Dagger")] = "dagger";
  itemTypes_[HashNameLower("Mace")] = "mace";
  itemTypes_[HashNameLower("Spear")] = "spear";
  itemTypes_[HashNameLower("Sword")] = "sword";
  itemTypes_[HashNameLower("CeremonialDagger")] = "ceremonialknife";
  itemTypes_[HashNameLower("FistWeapon")] = "fistweapon";
  itemTypes_[HashNameLower("Flail1H")] = "flail";
  itemTypes_[HashNameLower("MightyWeapon1H")] = "mightyweapon";
  itemTypes_[HashNameLower("Scythe1H")] = "scythe";
  itemTypes_[HashNameLower("Wand")] = "wand";
  itemTypes_[HashNameLower("HandXbow")] = "handcrossbow";
  itemTypes_[HashNameLower("Axe2H")] = "axe2h";
  itemTypes_[HashNameLower("Mace2H")] = "mace2h";
  itemTypes_[HashNameLower("Polearm")] = "polearm";
  itemTypes_[HashNameLower("Staff")] = "staff";
  itemTypes_[HashNameLower("Sword2H")] = "sword2h";
  itemTypes_[HashNameLower("CombatStaff")] = "daibo";
  itemTypes_[HashNameLower("Flail2H")] = "flail2h";
  itemTypes_[HashNameLower("MightyWeapon2H")] = "mightyweapon2h";
  itemTypes_[HashNameLower("Scythe2H")] = "scythe2h";
  itemTypes_[HashNameLower("Bow")] = "bow";
  itemTypes_[HashNameLower("Crossbow")] = "crossbow";
  itemTypes_[HashNameLower("Shield")] = "shield";
  itemTypes_[HashNameLower("CrusaderShield")] = "crusadershield";
  itemTypes_[HashNameLower("Orb")] = "source";
  itemTypes_[HashNameLower("Mojo")] = "mojo";
  itemTypes_[HashNameLower("Quiver")] = "quiver";
  itemTypes_[HashNameLower("NecromancerOffhand")] = "phylactery";
}

AffixValue const& GameAffixes::getAffix(uint32 id, bool recipe) {
  if (recipe) {
    auto it = instance().affixesRecipe_.find(id);
    if (it != instance().affixesRecipe_.end()) {
      return it->second.value;
    }
  }
  return instance().affixes_[id].value;
}
std::vector<AffixValue> GameAffixes::getGroup(uint32 id, uint32 itemType) {
  auto& itemTypeParent = instance().itemTypeParent_;
  std::vector<uint32> itemTypes;
  while (itemType != -1U) {
    itemTypes.push_back(itemType);
    auto it = itemTypeParent.find(itemType);
    if (it == itemTypeParent.end()) break;
    itemType = it->second;
  }
  std::map<std::pair<uint32, uint32>, AffixValue*> values;
  for (GameAffix* affix : instance().groups_[id]) {
    bool hasType = false;
    for (auto& type : itemTypes) {
      if (affix->itemTypes.count(type)) {
        hasType = true;
        break;
      }
    }
    if (hasType) {
      auto key = std::make_pair(affix->value.attributes[0].type, affix->value.attributes[0].param);
      auto it = values.find(key);
      if (it == values.end() || AffixValue::less(*it->second, affix->value)) {
        values[key] = &affix->value;
      }
    }
  }
  std::vector<AffixValue> result;
  for (auto& kv : values) {
    result.push_back(*kv.second);
  }
  return result;
}

GameAffixes& GameAffixes::instance() {
  static GameAffixes inst;
  return inst;
}

std::vector<std::string> GameAffixes::format(AttributeSpecifier const* begin, AttributeSpecifier const* end, FormatFlags flags) {
  auto& affixData = instance().affixData_;
  AttributeMap& defaultMap = instance().defaultMap_;
  DictionaryRef itemPowers = Strings::list("ItemPassivePowerDescriptions");
  DictionaryRef powers = Strings::list("Powers");
  DictionaryRef attributes = Strings::list("AttributeDescriptions");

  std::vector<std::string> result;
  std::map<std::string, AttributeMap> aggregate;
  for (; begin < end; ++begin) {
    uint32 type = fixAttrId(begin->type);
    if (!affixData.types.count(type)) continue;
    std::string affix = affixData.types[type];
    if (affix == "{Power}") {
      char const* power = Power::name(begin->param);
      if (power && itemPowers.has(power)) {
        affix = itemPowers[power];
      } else {
        continue;
      }
      AttributeMap map = defaultMap;
      map.emplace("value", begin->value);
      map.emplace("value1", begin->value);
      result.push_back(FormatDescription(affix, flags, map, PowerTags::getraw(begin->param)));
    } else if (affix[0] == '&') {
      char const* power = Power::name(begin->param);
      if (!power) continue;
      std::string name = fmtstring("%s_name", power);
      if (!powers.has(name)) continue;
      name = powers[name];
      affix = affix.substr(1);
      if (!attributes.has(affix)) continue;
      affix = attributes[affix];
      AttributeMap map = defaultMap;
      map.emplace("value1", name);
      map.emplace("value2", begin->value);
      if (strlower(affix).find("value3") != std::string::npos) {
        std::string pwr = strlower(power);
        for (auto& kv : affixData.raw["ResourceX"].getMap()) {
          if (pwr.find(kv.first) != std::string::npos) {
            map.emplace("value3", kv.second.getString());
            break;
          }
        }
      }
      result.push_back(FormatDescription(affix, flags, map));
    } else if (affix[0] == '@') {
      affix = affix.substr(1);
      AttributeMap map;
      map.emplace("value", begin->value);
      result.push_back(FormatDescription(affix, flags, map));
    } else {
      static re::Prog subst(R"(\{(\w+)\})");
      affix = subst.replace(affix, [&affixData, begin](re::Match const& m) {
        std::string key = m.group(1);
        std::string param = fmtstring("%d", begin->param);
        return affixData.raw[key][param].getString();
      });
      size_t dollar = affix.find('$');
      if (dollar != std::string::npos) {
        int index = atoi(affix.substr(dollar + 1).c_str());
        affix.resize(dollar);
        aggregate[affix][index ? fmtstring("value%d", index) : "value"] = begin->value;
      } else {
        if (!attributes.has(affix)) continue;
        affix = attributes[affix];
        AttributeMap map = defaultMap;
        map.emplace("value", begin->value);
        map.emplace("value1", begin->value);
        result.push_back(FormatDescription(affix, flags, map));
      }
    }
  }
  for (auto& am : aggregate) {
    if (!attributes.has(am.first)) continue;
    result.push_back(FormatDescription(attributes[am.first], flags, am.second));
  }
  return result;
}

std::vector<std::string> GameAffixes::format(std::vector<AttributeSpecifier> const& attrs, FormatFlags flags) {
  return format(attrs.data(), attrs.data() + attrs.size(), flags);
}

std::string GameAffixes::getItemType(uint32 id) {
  auto& types = instance().itemTypes_;
  auto& parent = instance().itemTypeParent_;
  while (!types.count(id) && parent.count(id)) {
    id = parent[id];
  }
  auto it = types.find(id);
  return (it == types.end() ? "" : it->second);
}
