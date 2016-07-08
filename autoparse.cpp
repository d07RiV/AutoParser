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
  std::vector<std::string> roots{ "C:/Work/junk/public_html" };
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

bool load_build(uint32 build) {
  auto builds = SnoCdnLoader::builds();
  SnoCdnLoader::BuildInfo info;
  for (std::string const& b : builds) {
    auto config = SnoCdnLoader::buildinfo(b);
    SnoCdnLoader::parsebuild(config, info);
    if (info.build == build) {
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
  json::write(File(path::work() / "versions.js", "wb"), versions);

  OpExtract(build);
  OpDumpPowers(build);
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
