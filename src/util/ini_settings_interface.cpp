#include "ini_settings_interface.h"
#include "common/file_system.h"
#include "common/log.h"
#include "common/string_util.h"
#include <algorithm>
#include <iterator>

Log_SetChannel(INISettingsInterface);

INISettingsInterface::INISettingsInterface(std::string filename) : m_filename(std::move(filename)), m_ini(true, true) {}

INISettingsInterface::~INISettingsInterface()
{
  if (m_dirty)
    Save();
}

bool INISettingsInterface::Load()
{
  if (m_filename.empty())
    return false;

  SI_Error err = SI_FAIL;
  auto fp = FileSystem::OpenManagedCFile(m_filename.c_str(), "rb");
  if (fp)
    err = m_ini.LoadFile(fp.get());

  return (err == SI_OK);
}

bool INISettingsInterface::Save()
{
  if (m_filename.empty())
    return false;

  SI_Error err = SI_FAIL;
  std::FILE* fp = FileSystem::OpenCFile(m_filename.c_str(), "wb");
  if (fp)
  {
    err = m_ini.SaveFile(fp, false);
    std::fclose(fp);
  }

  if (err != SI_OK)
  {
    Log_WarningPrintf("Failed to save settings to '%s'.", m_filename.c_str());
    return false;
  }

  m_dirty = false;
  return true;
}

void INISettingsInterface::Clear()
{
  m_ini.Reset();
}

bool INISettingsInterface::GetIntValue(const char* section, const char* key, s32* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  std::optional<s32> parsed_value = StringUtil::FromChars<s32>(str_value, 10);
  if (!parsed_value.has_value())
    return false;

  *value = parsed_value.value();
  return true;
}

bool INISettingsInterface::GetUIntValue(const char* section, const char* key, u32* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  std::optional<u32> parsed_value = StringUtil::FromChars<u32>(str_value, 10);
  if (!parsed_value.has_value())
    return false;

  *value = parsed_value.value();
  return true;
}

bool INISettingsInterface::GetFloatValue(const char* section, const char* key, float* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  std::optional<float> parsed_value = StringUtil::FromChars<float>(str_value);
  if (!parsed_value.has_value())
    return false;

  *value = parsed_value.value();
  return true;
}

bool INISettingsInterface::GetDoubleValue(const char* section, const char* key, double* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  std::optional<double> parsed_value = StringUtil::FromChars<double>(str_value);
  if (!parsed_value.has_value())
    return false;

  *value = parsed_value.value();
  return true;
}

bool INISettingsInterface::GetBoolValue(const char* section, const char* key, bool* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  std::optional<bool> parsed_value = StringUtil::FromChars<bool>(str_value);
  if (!parsed_value.has_value())
    return false;

  *value = parsed_value.value();
  return true;
}

bool INISettingsInterface::GetStringValue(const char* section, const char* key, std::string* value) const
{
  const char* str_value = m_ini.GetValue(section, key);
  if (!str_value)
    return false;

  value->assign(str_value);
  return true;
}

void INISettingsInterface::SetIntValue(const char* section, const char* key, int value)
{
  m_dirty = true;
  m_ini.SetLongValue(section, key, static_cast<long>(value), nullptr, false, true);
}

void INISettingsInterface::SetUIntValue(const char* section, const char* key, u32 value)
{
  m_dirty = true;
  m_ini.SetLongValue(section, key, static_cast<long>(value), nullptr, false, true);
}

void INISettingsInterface::SetFloatValue(const char* section, const char* key, float value)
{
  m_dirty = true;
  m_ini.SetDoubleValue(section, key, static_cast<double>(value), nullptr, true);
}

void INISettingsInterface::SetDoubleValue(const char* section, const char* key, double value)
{
  m_dirty = true;
  m_ini.SetDoubleValue(section, key, value, nullptr, true);
}

void INISettingsInterface::SetBoolValue(const char* section, const char* key, bool value)
{
  m_dirty = true;
  m_ini.SetBoolValue(section, key, value, nullptr, true);
}

void INISettingsInterface::SetStringValue(const char* section, const char* key, const char* value)
{
  m_dirty = true;
  m_ini.SetValue(section, key, value, nullptr, true);
}

bool INISettingsInterface::ContainsValue(const char* section, const char* key) const
{
  return (m_ini.GetValue(section, key, nullptr) != nullptr);
}

void INISettingsInterface::DeleteValue(const char* section, const char* key)
{
  m_dirty = true;
  m_ini.Delete(section, key);
}

void INISettingsInterface::ClearSection(const char* section)
{
  m_dirty = true;
  m_ini.Delete(section, nullptr);
  m_ini.SetValue(section, nullptr, nullptr);
}

std::vector<std::string> INISettingsInterface::GetStringList(const char* section, const char* key) const
{
  std::list<CSimpleIniA::Entry> entries;
  if (!m_ini.GetAllValues(section, key, entries))
    return {};

  std::vector<std::string> ret;
  ret.reserve(entries.size());
  for (const CSimpleIniA::Entry& entry : entries)
    ret.emplace_back(entry.pItem);
  return ret;
}

void INISettingsInterface::SetStringList(const char* section, const char* key, const std::vector<std::string>& items)
{
  m_dirty = true;
  m_ini.Delete(section, key);

  for (const std::string& sv : items)
    m_ini.SetValue(section, key, sv.c_str(), nullptr, false);
}

bool INISettingsInterface::RemoveFromStringList(const char* section, const char* key, const char* item)
{
  m_dirty = true;
  return m_ini.DeleteValue(section, key, item, true);
}

bool INISettingsInterface::AddToStringList(const char* section, const char* key, const char* item)
{
  std::list<CSimpleIniA::Entry> entries;
  if (m_ini.GetAllValues(section, key, entries) &&
      std::find_if(entries.begin(), entries.end(),
                   [item](const CSimpleIniA::Entry& e) { return (std::strcmp(e.pItem, item) == 0); }) != entries.end())
  {
    return false;
  }

  m_dirty = true;
  m_ini.SetValue(section, key, item, nullptr, false);
  return true;
}
