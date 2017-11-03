#include <stdio.h>
#include <iostream>
#include <clocale>
#include <set>
#include "snomap.h"
#include "strings.h"
#include "parser.h"
#include "json.h"
#include "path.h"
#include "image.h"
#include "textures.h"
#include "types/Textures.h"
#include "types/Appearance.h"
#include "types/Actor.h"
#include "types/Monster.h"
#include "types/AnimSet.h"
#include "types/StringList.h"
#include "types/SkillKit.h"
#include "regexp.h"
#include "translations.h"
#include "description.h"
#include "miner.h"
#include "powertag.h"
#include "itemlib.h"
#include "affixes.h"
#include <map>
#include <vector>
#include <algorithm>
#include <stdarg.h>

namespace path {
#ifdef _MSC_VER
  std::vector<std::string> roots{ "C:\\Work\\d3p\\Root" };
  std::vector<std::string> workdir{ "C:\\Work\\d3p\\Work" };
#else
  std::vector<std::string> roots { "/home/d3planner/public_html" };
#endif
}

void make_menu(File& mf, json::Value lhs, json::Value rhs) {
  struct {
    std::string fn, name;
  } classes[] = {
    { "Barbarian", "Barbarian" },
    { "Demonhunter", "Demon Hunter" },
    { "Monk", "Monk" },
    { "WitchDoctor", "Witch Doctor" },
    { "Wizard", "Wizard" },
    { "X1_Crusader", "Crusader" },
    { "Necromancer", "Necromancer" },
    { "Templar", "Templar" },
    { "Scoundrel", "Scoundrel" },
    { "Enchantress", "Enchantress" },
  };

  jsonCompare(rhs, lhs);
  std::set<istring> plist;
  for (auto& kv : rhs.getMap()) plist.insert(kv.first);
  for (auto& kv : lhs.getMap()) plist.insert(kv.first);
  Dictionary pown = readStrings("Powers");
  mf.printf("  <div class=\"menu\">\n");
  for (auto& cls : classes) {
    SnoFile<SkillKit> kit(cls.fn);
    std::vector<std::string> skills, traits;
    for (auto& skill : kit->x20_ActiveSkillEntries) {
      std::string power = skill.x00_PowerSno.name();
      if (plist.find(power) != plist.end()) {
        skills.push_back(fmtstring("   <a href=\"#%s\">%s</a>\n", power.c_str(), pown[power + "_name"].c_str()));
      }
    }
    for (auto& trait : kit->x10_TraitEntries) {
      std::string power = trait.x00_PowerSno.name();
      if (plist.find(power) != plist.end()) {
        traits.push_back(fmtstring("   <a href=\"#%s\">%s</a>\n", power.c_str(), pown[power + "_name"].c_str()));
      }
    }
    if (!skills.empty() || !traits.empty()) {
      mf.printf("   <h4>%s</h4>\n", cls.name.c_str());
      for (auto& str : skills) mf.printf("%s", str.c_str());
      if (!skills.empty() && !traits.empty()) mf.printf("   <br/>\n");
      for (auto& str : traits) mf.printf("%s", str.c_str());
    }
  }
  mf.printf("  </div>");
}

void make_diff(char const* name, uint32 lhs, uint32 rhs, std::vector<std::string> const& fields, bool links = false) {
  json::Value vlhs, vrhs;
  json::parse(File(path::work() / fmtstring("%s.%d.js", name, lhs)), vlhs);
  json::parse(File(path::work() / fmtstring("%s.%d.js", name, rhs)), vrhs);

  File out(path::root() / fmtstring("diff/%s.%d.%d.html", name, lhs, rhs), "wb");
  out.copy(File(path::work() / fmtstring("templates/diff.%s.html", name)));
  if (links) make_menu(out, vlhs, vrhs);
  diff(out, vlhs, vrhs, fields, links);
  out.copy(File(path::work() / "templates/diff.tail.html"));
}

void make_html(char const* name, uint32 version, json::Value const& value, std::vector<std::string> const& fields, bool links = false) {
  File out(path::root() / fmtstring("game/%s.%d.html", name, version), "wb");
  out.copy(File(path::work() / fmtstring("templates/game.%s.html", name)));
  makehtml(out, value, fields, links);
  out.copy(File(path::work() / "templates/game.tail.html"));
}

void write_icon(Archive& dsticons, GameBalance::Type::Item const& item) {
  static uint32 chestType = HashNameLower("ChestArmor");
  uint32 type = item.x10C_ItemTypesGameBalanceId;
  while (type && type != -1U && type != chestType) {
    type = GameAffixes::itemTypeParent(type);
  }
  bool torso = (type == chestType);

  SnoFile<Actor> actor(item.x108_ActorSno.name());
  if (!actor) return;
  auto& images = actor->x310_InventoryImages;
  std::set<uint32> icons;
  for (auto& inv : images) {
    if (inv.x00 != 0 && inv.x00 != -1) icons.insert(inv.x00);
    if (inv.x04 != 0 && inv.x04 != -1) icons.insert(inv.x04);
  }
  for (uint32 id : icons) {
    if (dsticons.has(id)) continue;
    Image image = GameTextures::get(id);
    if (image) {
      if (torso && (image.width() != 82 || image.height() != 164)) {
        Image tmp(82, 164);
        tmp.blt(41 - image.width() / 2, 82 - image.height() / 2, image);
        image = tmp;
      }
      image.write(dsticons.create(id), ImageFormat::PNG);
    }
  }
}

static json::Value versions;

template<class T>
std::vector<uint8> dump_bin(std::string const& name, uint32* usize = nullptr) {
  File src = SnoLoader::Load<T>(name);
  if (!src) return std::vector<uint8>();
  MemoryFile mem;
  json::WriterVisitor writer(mem);
  writer.setIndent(2);
  T::parse(src, &writer);
  writer.onEnd();
  std::vector<uint8> buf(mem.size());
  uint32 size = mem.size();
  gzencode(mem.data(), mem.size(), buf.data(), &size);
  buf.resize(size);
  if (usize) *usize = mem.size();
  return buf;
}

template<class T>
void dump_bin(File out) {
  uint32 count = SnoLoader::List<T>().size();
  std::vector<std::vector<uint8>> files;
  out.write32(count);
  uint32 total = 4 + (12 + 64) * count;
  for (std::string name : Logger::Loop(SnoLoader::List<T>(), fmtstring("Dumping %s", T::type()).c_str())) {
    uint32 usize;
    files.push_back(dump_bin<T>(name, &usize));
    out.write32(total);
    out.write32(usize);
    out.write32(files.back().size());
    name.resize(64, 0);
    out.write(name.data(), 64);
    total += files.back().size();
  }
  for (auto const& file : files) {
    out.write(file.data(), file.size());
  }
}

void OpGmb(uint32 build) {
  dump_bin<GameBalance>(File(fmtstring("GameBalance.%d.gz", build), "wb"));
}

void OpExtract(uint32 build) {
  std::string suffix = fmtstring(".%d.js", build);
  json::Value js_sets;
  json::Value js_items;
  json::Value js_powers;
  for (auto& gmb : SnoLoader::All<GameBalance>()) {
    for (auto& val : gmb->x168_SetItemBonusTable) {
      parseSetBonus(val, js_sets, false);
    }
    for (auto& val : gmb->x028_Items) {
      parseItem(val, js_items, false);
    }
  }
  for (auto& pow : SnoLoader::All<Power>()) {
    parsePower(*pow, js_powers, false);
  }
  json::write(File(path::work() / "itemsets" + suffix, "w"), js_sets);
  json::write(File(path::work() / "items" + suffix, "w"), js_items);
  json::write(File(path::work() / "powers" + suffix, "w"), js_powers);

  make_html("items", build, js_items, { "name", "set", "icon", "flavor", "primary", "secondary", "powers" });
  make_html("itemsets", build, js_sets, { "name", "2", "3", "4", "5", "6", "powers" });
}

void OpCompare(uint32 build, uint32 other) {
  make_diff("items", other, build, { "name", "set", "icon", "flavor", "primary", "secondary", "powers" });
  make_diff("itemsets", other, build, { "name", "2", "3", "4", "5", "6", "powers" });
  make_diff("powers", other, build, { "name", "desc", "flavor", "rune_a", "rune_b", "rune_c", "rune_d", "rune_e" }, true);
}

void OpExtractIcons() {
  Archive icons;
  for (auto& gmb : SnoLoader::All<GameBalance>()) {
    for (auto& item : gmb->x028_Items) {
      write_icon(icons, item);
    }
  }
  icons.write(File("icons.wgz", "wb"), false);
}

void OpDumpPowers(uint32 build) {
  json::Value value = PowerTags::dump();
  File out(path::root() / fmtstring("powers/jspowers.%d.js", build), "wb");
  out.printf("var Powers=");
  json::WriterVisitor visitor(out, json::mJS);
  value.walk(&visitor);
  visitor.onEnd();
  out.printf(";");
}


static std::map<std::string, std::string> slotSizes{
  {"head", "default"},
  {"shoulders", "default"},
  {"neck", "square"},
  {"torso", "big"},
  {"waist", "square"},
  {"hands", "default"},
  {"wrists", "default"},
  {"legs", "default"},
  {"feet", "default"},
  {"finger", "square"},
  {"onehand", "default"},
  {"twohand", "default"},
  {"offhand", "default"},
  {"follower", "square"},
  {"custom", "square"},
  {"customwpn", "square"},
};

static std::map<std::string, int> classToIcon{
  {"demonhunter", 0},
  {"barbarian", 2},
  {"wizard", 4},
  {"witchdoctor", 6},
  {"monk", 8},
  {"crusader", 10},
  {"necromancer", 12},
};

static std::vector<std::string> bgEffects{ "physical", "fire", "lightning", "cold", "poison", "arcane", "holy" };

static std::string getItemIcon(json::Value const& icons, GameBalance::Type::Item const* item) {
  auto icon = icons[item->x000_Text][1];
  std::string base = "http://www.d3planner.com/webgl/icons/";
  if (icon.type() == json::Value::tNumber || icon.type() == json::Value::tInteger) {
    return base + fmtstring("%.0f", icon.getNumber());
  } else {
    uint32 idxClass = 0;
    while (idxClass < 7 && !(&item->x3C8)[idxClass]) {
      ++idxClass;
    }
    if (idxClass >= 7) idxClass = 0;
    uint32 idxGender = (idxClass == 0 || idxClass == 2 ? 1 : 0);
    idxClass *= 2;
    double iconIndex = 0;
    if (icon.type() == json::Value::tArray) {
      if (icon.length() > idxClass + idxGender) iconIndex = icon[idxClass + idxGender].getNumber();
      else if (icon.length() > idxClass) iconIndex = icon[idxClass].getNumber();
      else if (icon.length() > idxGender) iconIndex = icon[idxGender].getNumber();
      else iconIndex = icon[0].getNumber();
    } else if (icon.type() == json::Value::tObject) {
      if (icon.has(fmtstring("%d", idxClass + idxGender))) iconIndex = icon[fmtstring("%d", idxClass + idxGender)].getNumber();
      else if (icon.has(fmtstring("%d", idxClass))) iconIndex = icon[fmtstring("%d", idxClass)].getNumber();
      else if (icon.has(fmtstring("%d", idxGender))) iconIndex = icon[fmtstring("%d", idxGender)].getNumber();
      else iconIndex = icon["0"].getNumber();
    }
    return base + fmtstring("%.0f", iconIndex);
  }
}

void OpKadala(uint32 build) {
  static re::Prog re_sockets(R"(Sockets \((\d+)\))");
  static re::Prog re_random(R"(\+(\d+) Random Magic Properties)");
  static std::map<uint32, std::vector<GameBalance::Type::Item*>> itemSets;
  static json::Value setData;
  static re::Prog getname(R"((.*) \([0-9]+\))");
  static std::vector<std::string> classList{ "demonhunter", "barbarian", "wizard", "witchdoctor", "monk", "crusader", "necromancer" };

  json::Value input, output, icons;
  json::parse(File("item_data.js"), input, json::mJS);
  json::parse(File("item_icons.js"), icons, json::mJS);
  json::parse(File("kadala_src.js"), output, json::mJS);
  auto names = Strings::list("Items");
  auto limits = input["statLimits"]["legendary"];
  for (auto& kv : ItemLibrary::all()) {
    std::string id = kv.first;
    auto* item = kv.second;
    if (!item->x3C4) continue;

    std::string type = GameAffixes::getItemType(item->x10C_ItemTypesGameBalanceId.id);
    if (!input["itemTypes"].has(type)) continue;
    auto typeInfo = input["itemTypes"][type];

    json::Value& dst = output["Items"][id];
    dst["icon"] = icons[id][0].getInteger();
    dst["level"] = item->x14C;
    dst["weight"] = item->x3C4;
    dst["name"] = names[id];
    dst["type"] = type;
    if (item->x110_Bit18) dst["hardcore"] = true;
    int total_classes = 0;
    int last_class = 0;
    for (int i = 0; i < 7; ++i) {
      if ((&item->x3C8)[i]) {
        ++total_classes;
        last_class = i;
        dst[classList[i]] = true;
      }
    }

    json::Value parsed;
    auto info = parseItem(*item, parsed, true, true);
    parsed = parsed[id];

    std::string itemClass;
    if (total_classes == 1) {
      itemClass = classList[last_class];
    } else if (typeInfo.has("class")) {
      itemClass = typeInfo["class"].getString();
    }
    std::string itemSlot = typeInfo["slot"].getString();
    std::string slotName;
    if (input["metaSlots"].has(itemSlot)) {
      slotName = input["metaSlots"][itemSlot]["name"].getString();
    } else {
      slotName = input["itemSlots"][itemSlot]["name"].getString();
    }

    std::string color = (item->x170_SetItemBonusesGameBalanceId.id == -1U ? "orange" : "green");
    std::string effect = "";// effect - bg effect - bg - holy";
    if (info.elemental && static_cast<size_t>(info.elemental) < bgEffects.size() && slotSizes[itemSlot] != "square") {
      effect = " effect-bg effect-bg-" + bgEffects[info.elemental];
    } else if (typeInfo["required"].has("basearmor")) {
      effect = " effect-bg effect-bg-armor";
      if (slotSizes[itemSlot] != "default") {
        effect += " effect-bg-armor-" + slotSizes[itemSlot];
      }
    }

    std::string tooltip = "<div class=\"d3-tooltip d3-tooltip-item\"><div class=\"d3-tooltip-item-wrapper d3-tooltip-item-wrapper-Legendary\">"
      "<div class=\"d3-tooltip-item-border d3-tooltip-item-border-left\"></div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-right\">"
      "</div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-top\"></div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-bottom\">"
      "</div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-top-left\"></div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-top-right\">"
      "</div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-bottom-left\"></div><div class=\"d3-tooltip-item-border d3-tooltip-item-border-bottom-right\">"
      "</div><div class=\"tooltip-head tooltip-head-orange\">";
    tooltip += "<h3 class=\"d3-color-" + color + "\">";
    tooltip += names[id];
    tooltip += "</h3></div><div class=\"tooltip-body" + effect + "\"><span class=\"d3-icon d3-icon-item d3-icon-item-large  d3-icon-item-" + color + "\">"
      "<span class=\"icon-item-gradient\"><span class=\"icon-item-inner icon-item-" + slotSizes[itemSlot] + "\" "
      "style=\"background-image: url(" + getItemIcon(icons, item) + ");\">"
      "</span></span></span><div class=\"d3-item-properties\"><ul class=\"item-type-right\">";
    tooltip += "<li class=\"item-slot\">" + slotName + "</li>";
    if (itemClass.size()) {
      tooltip += "<li class=\"item-class-specific d3-color-white\">" + input["classes"][itemClass]["name"].getString() + "</li>";
    }
    tooltip += "</ul><ul class=\"item-type\"><li><span class=\"d3-color-" + color += "\">";
    tooltip += (item->x170_SetItemBonusesGameBalanceId.id == -1U ? "Legendary " : "Set ") + typeInfo["name"].getString();
    tooltip += "</span></li></ul>";
    if (typeInfo["required"].has("basearmor")) {
      auto armor = limits[typeInfo["required"]["basearmor"].getString()];
      int min = armor["min"].getInteger();
      int max = armor["max"].getInteger();
      for (auto const& attr : info.attrs) {
        if (fixAttrId(attr.type) == 34 || fixAttrId(attr.type) == 35) {
          min += attr.value.min;
          max += attr.value.max;
        }
      }
      std::string armorStr = (min < max ? fmtstring("%d &#x2013; %d", min, max) : fmtstring("%d", min));
      tooltip += "<ul class=\"item-armor-weapon item-armor-armor\"><li class=\"big\"><p><span class=\"value\">" + armorStr + "</span></p></li><li>Armor</li></ul>";
    }
    if (typeInfo.has("weapon")) {
      double speed = typeInfo["weapon"]["speed"].getNumber();
      int min_min = typeInfo["weapon"]["min"].getInteger(), min_max = min_min;
      int delta_min = typeInfo["weapon"]["max"].getInteger() - min_min, delta_max = delta_min;
      double damage_min = 0, damage_max = 0;
      double speed_min = 0, speed_max = 0;
      for (auto const& attr : info.attrs) {
        int id = fixAttrId(attr.type);
        if (id == 211 || id == 228 || id == 235 || id == 234) {
          min_min += attr.value.min;
          min_max += attr.value.max;
        } else if (id == 209 || id == 219 || id == 227 || id == 226) {
          delta_min += attr.value.min;
          delta_max += attr.value.max;
        } else if (id == 238) {
          damage_min += attr.value.min;
          damage_max += attr.value.max;
        } else if (id == 200) {
          speed_min += attr.value.min;
          speed_max += attr.value.max;
        }
      }
      double fin_speed_min = speed * (1.00 + speed_min), fin_speed_max = speed * (1.00 + speed_max);
      double fin_min_min = min_min * (1.00 + damage_min), fin_min_max = min_max * (1.00 + damage_max);
      double fin_max_min = (min_min + delta_min) * (1.00 + damage_min), fin_max_max = (min_max + delta_max) * (1.00 + damage_max);
      std::string cls = "big";
      std::string dps = fmtstring("%.1f", (fin_min_min + fin_max_min) * fin_speed_min * 0.5);
      if (fin_min_max > fin_min_min || fin_max_max > fin_max_min || fin_speed_max > fin_speed_min) {
        cls = "med";
        dps += fmtstring("&#x2013;%.1f", (fin_min_max + fin_max_max) * fin_speed_max * 0.5);
      }
      std::string txt_min = (fin_min_max > fin_min_min ? fmtstring("(%.0f&#x2013;%.0f)", fin_min_min, fin_min_max) : fmtstring("%.0f", fin_min_min));
      std::string txt_max = (fin_max_max > fin_max_min ? fmtstring("(%.0f&#x2013;%.0f)", fin_max_min, fin_max_max) : fmtstring("%.0f", fin_max_min));
      std::string txt_speed = (fin_speed_max > fin_speed_min ? fmtstring("%.2f&#x2013;%.2f", fin_speed_min, fin_speed_max) : fmtstring("%.2f", fin_speed_min));

      tooltip += "<ul class=\"item-armor-weapon item-weapon-dps\"><li class=\"" + cls + "\"><span class=\"value\">" + dps + "</span></li><li>Damage Per Second</li></ul>";
      tooltip += "<ul class=\"item-armor-weapon item-weapon-damage\"><li><p><span class=\"value\">" + txt_min +
        "</span>&#x2013;<span class=\"value\">" + txt_max + "</span> <span class=\"d3-color-FF888888\">Damage</span></p></li>"
        "<li><p><span class=\"value\">" + txt_speed + "</span> <span class=\"d3-color-FF888888\">Attacks per Second</span></p></li></ul>";
    }
    if (typeInfo["required"].has("baseblock") && typeInfo["required"].has("blockamount")) {
      auto block = limits[typeInfo["required"]["baseblock"].getString()];
      auto amount = limits[typeInfo["required"]["blockamount"].getString()];
      double block_min = block["min"].getNumber();
      double block_max = block["max"].getNumber();
      for (auto const& attr : info.attrs) {
        if (fixAttrId(attr.type) == 260) {
          block_min += attr.value.min;
          block_max += attr.value.max;
        }
      }
      std::string str_block = (block_min < block_max ? fmtstring("%.1f&#x2013;%.1f", block_min, block_max) : fmtstring("%.1f", block_min));
      std::string str_amount = fmtstring("<span class=\"value\">(%d&#x2013;%d)</span>&#x2013;<span class=\"value\">(%d&#x2013;%d)</span>",
        amount["min"].getInteger(), amount["max"].getInteger(), amount["min2"].getInteger(), amount["max2"].getInteger());
      tooltip += "<ul class=\"item-armor-weapon item-armor-shield\"><li><p><span class=\"value\">+" + str_block +
        "%</span> <span class=\"d3-color-FF888888\">Chance to Block</span></p></li><li><p><span class=\"value\">" + str_amount +
        "</span> <span class=\"d3-color-FF888888\">Block Amount</span></p></li></ul>";
    }

    tooltip += "<div class=\"item-before-effects\"></div><ul class=\"item-effects\">";
    int sockets = 0, random = 0, primary = 0, secondary = 0;
    std::vector<json::Value> groups;
    if (parsed.has("primary")) {
      for (auto const& attr : parsed["primary"].getArray()) {
        std::vector<std::string> sub;
        if (attr.type() == json::Value::tArray) {
          groups.push_back(attr);
        } else if (re_sockets.find(attr.getString(), 0, &sub) >= 0) {
          sockets = std::stoi(sub[1]);
        } else if (re_random.find(attr.getString(), 0, &sub) >= 0) {
          random += std::stoi(sub[1]);
        } else {
          if (!primary++) tooltip += "<p class=\"item-property-category\">Primary</p>";
          tooltip += attr.getString();
        }
      }
    }
    if (parsed.has("secondary")) {
      for (auto const& attr : parsed["secondary"].getArray()) {
        std::vector<std::string> sub;
        //if (attr.type() == json::Value::tArray) {
        if (attr.type() == json::Value::tArray) {
          groups.push_back(attr);
        } else if (re_random.find(attr.getString(), 0, &sub) >= 0) {
          random += std::stoi(sub[1]);
        } else {
          if (!secondary++) tooltip += "<p class=\"item-property-category\">Secondary</p>";
          tooltip += attr.getString();
        }
      }
    }

    if (random || groups.size()) {
      tooltip += "<br/>";
      for (auto const& group : groups) {
        if (group.length() > 10) {
          ++random;
          continue;
        }
        tooltip += fmtstring("<li class=\"item-effects-choice\"><span class=\"d3-color-blue\">One of <span class=\"value\">%d</span> Magic Properties (varies)</span><ul>", group.length());
        for (auto const& sub : group.getArray()) {
          tooltip += sub.getString();
        }
        tooltip += "</ul></li>";
      }
      if (random) {
        tooltip += "<li class=\"d3-color-blue\"><span class=\"value\">+" + fmtstring("%d", random) + "</span> Random Magic Properties</li>";
      }
    }

    for (int i = 0; i < sockets; ++i) {
      tooltip += "<li class=\"empty-socket d3-color-blue\">Empty Socket</li>";
    }

    tooltip += "</ul>";

    if (parsed.has("set")) {
      if (itemSets.empty()) {
        for (auto const& kv : ItemLibrary::all()) {
          if (SnoManager::gameBalance()[kv.second->x170_SetItemBonusesGameBalanceId]) {
            itemSets[kv.second->x170_SetItemBonusesGameBalanceId.id].push_back(kv.second);
          }
        }

        SnoFile<GameBalance> sets("SetItemBonuses");
        for (auto& it : sets->x168_SetItemBonusTable) {
          parseSetBonus(it, setData, true);
        }
      }

      std::map<std::string, std::string> setItems;
      for (auto const& sub : itemSets[item->x170_SetItemBonusesGameBalanceId.id]) {
        if (!input["itemById"].has(sub->x000_Text)) continue;
        std::string itype = input["itemById"][sub->x000_Text]["type"].getString();
        std::string islot = input["itemTypes"][itype]["slot"].getString();
        if (input["metaSlots"].has(islot)) {
          islot = input["metaSlots"][islot]["name"].getString();
        } else {
          islot = input["itemSlots"][islot]["name"].getString();
        }
        setItems[itype] = fmtstring("%s (%s)", names[sub->x000_Text].c_str(), islot.c_str());
      }

      tooltip += "<ul class=\"item-itemset\"><li class=\"item-itemset-name\"><span class=\"d3-color-green\">" + parsed["set"].getString() + "</span></li>";
      for (auto const& sub : setItems) {
        tooltip += "<li class=\"item-itemset-piece indent\"><span class=\"d3-color-gray\">" + sub.second + "</span></li>";
      }

      std::string name = getname.replace(item->x170_SetItemBonusesGameBalanceId.name(), "\\1");
      if (setData.has(name)) {
        for (int i = 1; i < 10; ++i) {
          std::string key = fmtstring("%d", i);
          if (setData[name].has(key)) {
            auto list = setData[name][key];
            tooltip += "<li class=\"d3-color-gray item-itemset-bonus-amount\">(<span class=\"value\">" + key + "</span>) Set:</li>";
            for (auto const& bonus : list.getArray()) {
              tooltip += "<li class=\"d3-color-gray item-itemset-bonus-desc indent\">" + bonus.getString() + "</li>";
            }
          }
        }
      }
      tooltip += "</ul>";
    }

    tooltip += "<ul class=\"item-extras\"><li class=\"item-reqlevel\"><span class=\"d3-color-gold\">Required Level: </span><span class=\"value\">70</span></li>";
    tooltip += "<li>Account Bound</li></ul><span class=\"item-unique-equipped\">Unique Equipped</span><span class=\"clear\"><!--   --></span></div></div>";

    if (parsed.has("flavor")) {
      tooltip += "<div class=\"tooltip-extension \"><div class=\"flavor\">" + parsed["flavor"].getString() + "</div></div>";
    }

    tooltip += "</div></div>";

    dst["tooltip"] = tooltip;

    //"<div class=\"tooltip-extension \"><div class=\"flavor\">A mighty symbol of Tyrael, the archangel of Justice, though never actually used by him. Stolen from the Courts of Justice in the High Heavens by renegade angels.</div></div></div></div>"
  }

  File fout(fmtstring("kadala.%d.js", build), "wb");
  fout.printf("var Kadala = ");
  json::write(fout, output, json::mJS);
  fout.seek(-1, SEEK_CUR);
  fout.printf(";\n");
}

uint32 build_id(std::string const& hash) {
  auto config = SnoCdnLoader::buildinfo(hash);
  SnoCdnLoader::BuildInfo info;
  SnoCdnLoader::parsebuild(config, info);
  return info.build;
}

bool load_build(uint32 build) {
  auto builds = SnoCdnLoader::builds();
  for (std::string const& b : builds) {
    if (build_id(b) == build) {
      try {
        SnoLoader::primary = new SnoCdnLoader(b, "enUS");
        return true;
      } catch (Exception& e) {
      }
    }
  }
  return false;
}

int do_main(uint32 build) {
  if (File::exists(path::work() / fmtstring("items.%d.js", build))) {
    Logger::log("Version %d already loaded", build);
    return 0;
  }

  if (!load_build(build)) {
    Logger::log("Failed to find build %d", build);
    return 1;
  }

  //SnoLoader::primary->dump<GameBalance>();
  //return 0;

  json::Value versions;
  json::parse(File(path::work() / "versions.js"), versions);
  uint32 prevVersion = 0;
  for (auto const& kv : versions.getMap()) {
    uint32 ver = std::stoi(kv.first);
    if (ver < build && ver > prevVersion && File::exists(path::work() / fmtstring("items.%d.js", ver))) {
      prevVersion = ver;
    }
  }
  versions[fmtstring("%d", build)] = SnoLoader::primary->version();
/*  uint32 live_id = 0, ptr_id = 0;
  auto all_builds = SnoCdnLoader::builds();
  if (all_builds.size() >= 1) live_id = build_id(all_builds[0]);
  if (all_builds.size() >= 2) ptr_id = build_id(all_builds[1]);
  for (auto const& kv : versions.getMap()) {
    std::string name = kv.second.getString();
    size_t space = name.find(' ');
    if (space != std::string::npos) {
      name = name.substr(0, space);
    }
    uint32 id = std::atoi(kv.first.c_str());
    if (id == live_id) {
      name.append(" (Live)");
    } else if (id == ptr_id) {
      name.append(" (PTR)");
    }
    versions[kv.first] = name;
  }*/
  json::write(File(path::work() / "versions.js", "wb"), versions);

  OpGmb(build);
  OpExtract(build);
  OpDumpPowers(build);
  OpKadala(build);
  json::write(File(path::root() / "game/versions.js", "wb"), versions);
  json::write(File(path::root() / "diff/versions.js", "wb"), versions);
  json::write(File(path::root() / "powers/versions.js", "wb"), versions);

  if (prevVersion) {
    OpCompare(build, prevVersion);
  }
  SnoCdnLoader::BuildInfo liveinfo;
  if (SnoCdnLoader::parsebuild(SnoCdnLoader::buildinfo(SnoCdnLoader::livebuild()), liveinfo)) {
    if (liveinfo.build != build && liveinfo.build != prevVersion && File::exists(path::work() / fmtstring("items.%d.js", liveinfo.build))) {
      OpCompare(build, liveinfo.build);
    }
  }

  return 0;
}

#ifndef _MSC_VER
#include <sys/file.h>
#include <errno.h>
#endif

int main(int argc, const char** argv) {
  std::setlocale(LC_ALL, "eu_US.UTF-8");

  /*try {
    SnoCdnLoader::convert();
    return 0;
  } catch (Exception& ex) {
    fprintf(stderr, "Exception: %s\n", ex.what());
    return 1;
  }*/

#ifndef _MSC_VER
  int pid_file = open("autoparse.pid", O_CREAT | O_RDWR, 0666);
  int rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    fprintf(stderr, "Another instance is running, exiting . . .\n");
    return 0;
  }
#endif

  if (argc < 2) {
    fprintf(stderr, "No arguments given, exiting . . .\n");
    return 1;
  }

  try {
    return do_main(std::atoi(argv[1]));
  } catch (Exception& ex) {
    fprintf(stderr, "Exception: %s\n", ex.what());
    return 1;
  }
}
