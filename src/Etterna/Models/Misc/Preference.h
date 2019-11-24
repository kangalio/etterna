﻿/* Preference - Holds user-chosen preferences that are saved between sessions.
 */

#ifndef PREFERENCE_H
#define PREFERENCE_H

#include "EnumHelper.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"
class XNode;

struct lua_State;
class IPreference
{
  public:
	IPreference(const RString& sName);
	virtual ~IPreference();
	void ReadFrom(const XNode* pNode, bool bIsStatic);
	void WriteTo(XNode* pNode) const;
	void ReadDefaultFrom(const XNode* pNode);

	virtual void LoadDefault() = 0;
	virtual void SetDefaultFromString(const RString& s) = 0;

	virtual RString ToString() const = 0;
	virtual void FromString(const RString& s) = 0;

	virtual void SetFromStack(lua_State* L);
	virtual void PushValue(lua_State* L) const;

	const RString& GetName() const { return m_sName; }

	static IPreference* GetPreferenceByName(const RString& sName);
	static void LoadAllDefaults();
	static void ReadAllPrefsFromNode(const XNode* pNode, bool bIsStatic);
	static void SavePrefsToNode(XNode* pNode);
	static void ReadAllDefaultsFromNode(const XNode* pNode);

	RString GetName() { return m_sName; }
	void SetStatic(bool b) { m_bIsStatic = b; }

  private:
	RString m_sName;
	bool m_bIsStatic; // loaded from Static.ini?  If so, don't write to
					  // Preferences.ini
};

void
BroadcastPreferenceChanged(const RString& sPreferenceName);

template<class T>
class Preference : public IPreference
{
  public:
	Preference(const RString& sName,
			   const T& defaultValue,
			   void(pfnValidate)(T& val) = NULL)
	  : IPreference(sName)
	  , m_currentValue(defaultValue)
	  , m_defaultValue(defaultValue)
	  , m_pfnValidate(pfnValidate)
	{
		LoadDefault();
	}

	RString ToString() const override
	{
		return StringConversion::ToString<T>(m_currentValue);
	}
	void FromString(const RString& s) override
	{
		if (!StringConversion::FromString<T>(s, m_currentValue))
			m_currentValue = m_defaultValue;
		if (m_pfnValidate)
			m_pfnValidate(m_currentValue);
	}
	void SetFromStack(lua_State* L) override
	{
		LuaHelpers::Pop<T>(L, m_currentValue);
		if (m_pfnValidate)
			m_pfnValidate(m_currentValue);
	}
	void PushValue(lua_State* L) const override
	{
		LuaHelpers::Push<T>(L, m_currentValue);
	}

	void LoadDefault() override { m_currentValue = m_defaultValue; }
	void SetDefaultFromString(const RString& s) override
	{
		T def = m_defaultValue;
		if (!StringConversion::FromString<T>(s, m_defaultValue))
			m_defaultValue = def;
	}

	const T& Get() const { return m_currentValue; }

	const T& GetDefault() const { return m_defaultValue; }

	operator const T() const { return Get(); }

	void Set(const T& other)
	{
		m_currentValue = other;
		BroadcastPreferenceChanged(GetName());
	}

	static Preference<T>* GetPreferenceByName(const RString& sName)
	{
		IPreference* pPreference = IPreference::GetPreferenceByName(sName);
		Preference<T>* pRet = dynamic_cast<Preference<T>*>(pPreference);
		return pRet;
	}

  private:
	T m_currentValue;
	T m_defaultValue;
	void (*m_pfnValidate)(T& val);
};

/** @brief Utilities for working with Lua. */
namespace LuaHelpers {
template<typename T>
void
Push(lua_State* L, const Preference<T>& Object)
{
	LuaHelpers::Push<T>(L, Object.Get());
}
}

template<class T>
class Preference1D
{
  public:
	using PreferenceT = Preference<T>;
	vector<PreferenceT*> m_v;

	Preference1D(void pfn(size_t i, RString& sNameOut, T& defaultValueOut),
				 size_t N)
	{
		for (size_t i = 0; i < N; ++i) {
			RString sName;
			T defaultValue;
			pfn(i, sName, defaultValue);
			m_v.push_back(new Preference<T>(sName, defaultValue));
		}
	}

	~Preference1D()
	{
		for (size_t i = 0; i < m_v.size(); ++i)
			SAFE_DELETE(m_v[i]);
	}
	const Preference<T>& operator[](size_t i) const { return *m_v[i]; }
	Preference<T>& operator[](size_t i) { return *m_v[i]; }
};

#endif