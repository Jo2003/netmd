#include <algorithm>
#include "CMDiscHeader.hpp"

//-----------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//-----------------------------------------------------------------------------
CMDiscHeader::CMDiscHeader() 
	:mGroupId(0)
{
	// add title entry
	mGroups.push_back({mGroupId++, 0, -1, ""});
}

//-----------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//!
//! @param[in]  header  The RAW disc header as string
//-----------------------------------------------------------------------------
CMDiscHeader::CMDiscHeader(const std::string& header)
	:mGroupId(0)
{
	fromString(header);
}

//-----------------------------------------------------------------------------
//! @brief      create header from string
//!
//! @param[in]  header  The RAW disc header as string
//!
//! @return     { description_of_the_return_value }
//-----------------------------------------------------------------------------
int CMDiscHeader::fromString(const std::string& header)
{
	mGroups.clear();

	if (header.empty())
	{
		mGroups.push_back({mGroupId++, 0, -1, ""});
	}
	else if (header.find("//") == std::string::npos)
	{
		mGroups.push_back({mGroupId++, 0, -1, header});
	}
	else
	{
		std::size_t pos = 0, num, end, dash;
		std::string tok, numb;
		Group_t group;

		while ((end = header.find("//", pos)) != std::string::npos)
		{
			group.mFirst = -1;
			group.mLast  = -1;
				
			tok = header.substr(pos, end - pos);
			std::cout << "Parse token '" << tok << "'!" << std::endl;
			pos = end + 2;
			num = tok.find(';');
			
			group.mName = tok.substr(num + 1);
			std::cout << "Group name '" << group.mName << "'!" << std::endl;

			if ((num != std::string::npos) && (num > 0))
			{
				numb = tok.substr(0, num);

				if ((dash = numb.find('-')) != std::string::npos)
				{
					group.mFirst = atoi(numb.substr(0, dash).c_str());
					group.mLast  = atoi(numb.substr(dash + 1).c_str());
				}
				else
				{
					group.mFirst = atoi(numb.c_str());
				}
			}

			group.mGid = mGroupId++;
			mGroups.push_back(group);
		}
	}

	return sanityCheck(mGroups);
}

//-----------------------------------------------------------------------------
//! @brief      check groups / tracks for sanity
//!
//! @param[in]  grps  The grps
//!
//! @return     0 -> all well; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::sanityCheck(const Groups_t& grps) const
{
	int last = 0, ret = 0;

	Groups_t tmpGrps = grps;

	std::sort(tmpGrps.begin(), tmpGrps.end(), 
		[](const Group_t& a, const Group_t& b)->bool
		{
			return a.mFirst < b.mFirst;
		}
	);

	for(const auto& g : tmpGrps)
	{
		if ((g.mFirst == 0) && (g.mLast != -1))
		{
			std::cerr << "Title group can't have a last entry!" << std::endl;
			ret = -1;
			break;
		}
		else if ((g.mFirst == -1) && (g.mLast != -1))
		{
			std::cerr << "An empty group can't have a last entry!" << std::endl;
			ret = -1;
			break;
		}
		else if ((g.mFirst > g.mLast) && (g.mLast != -1))
		{
			std::cerr << "First track number can't be larger than last track number!" << std::endl;
			ret = -1;
			break;
		}
		else if ((g.mFirst > 0) && (g.mFirst <= last))
		{
			std::cerr << "Some groups share the same track numbers!" << std::endl;
			ret = -1;
			break;
		}

		last  = (g.mLast == -1) ? g.mFirst : g.mLast;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//! @brief      Returns a string representation of the object.
//!
//! @return     String representation of the object.
//-----------------------------------------------------------------------------
std::string CMDiscHeader::toString()
{
	mOss.clear();
	mOss.str("");

	Groups_t tmpGrps = mGroups;

	Group_t& title = tmpGrps.at(0);

	if ((title.mFirst == 0) && !title.mName.empty())
	{
		if (tmpGrps.size() == 1)
		{
			mOss << title.mName;
			return mOss.str();
		}
		else
		{
			mOss << "0;" << title.mName << "//";
			tmpGrps.erase(tmpGrps.begin());
		}
	}

	std::sort(tmpGrps.begin(), tmpGrps.end(), 
		[](const Group_t& a, const Group_t& b)->bool
		{
			return a.mFirst < b.mFirst;
		}
	);

	for (const auto& g : tmpGrps)
	{
		if (g.mFirst != -1)
		{
			mOss << g.mFirst;
		}

		if (g.mLast != -1)
		{
			mOss << "-" << g.mLast;
		}

		mOss << ";" << g.mName << "//";
	}

	return mOss.str();
}

//-----------------------------------------------------------------------------
//! @brief      Adds a group.
//!
//! @param[in]  name   The name
//! @param[in]  first  The first track (optional)
//! @param[in]  last   The last track (otional)
//!
//! @return     >= 0 -> group idx; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::addGroup(const std::string& name, int16_t first, int16_t last)
{
	Groups_t tmpGrps = mGroups;
	tmpGrps.push_back({mGroupId++, first, last, name});

	if (sanityCheck(tmpGrps) == 0)
	{
		mGroups = tmpGrps;
		return mGroupId - 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      list/print all groups
//-----------------------------------------------------------------------------
void CMDiscHeader::listGroups() const
{
	for (const auto& g : mGroups)
	{
		std::cout << "Group " << g.mGid << " (" << g.mName << ")";

		if (g.mFirst == 0)
		{
			std::cout << ", disc title";
		}
		
		if (g.mFirst > 0)
		{
			std::cout << ", track(s) " << g.mFirst;
		}
		
		if (g.mLast != -1)
		{
			std::cout << " - " << g.mLast;
		}

		std::cout << std::endl;
	}
}

//-----------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::addTrackToGroup(int gid, int16_t track)
{
	Groups_t tmpGrps = mGroups;
	int16_t first, last;
	bool changed = false;

	for (auto& g : tmpGrps)
	{
		if (g.mGid == gid)
		{
			if ((g.mFirst == -1) && (g.mLast == -1))
			{
				g.mFirst = track;
				changed  = true;
				break;
			}

			first = g.mFirst;
			last  = (g.mLast == -1) ? first : g.mLast;

			if ((first - track) == 1)
			{
				g.mFirst = track;
				changed  = true;
			}
			else if ((track - last) == 1)
			{
				g.mLast = track;
				changed  = true;
			}
			break;
		}
	}

	if (changed && (sanityCheck(tmpGrps) == 0))
	{
		mGroups = tmpGrps;
		return 0;
	}

	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::delTrackFromGroup(int gid, int16_t track)
{
	Groups_t tmpGrps = mGroups;
	int16_t first, last;
	bool changed = false;

	for (auto& g : tmpGrps)
	{
		if (g.mGid == gid)
		{
			if ((track == g.mFirst) && (g.mLast == -1))
			{
				g.mFirst = -1;
				changed  = true;
				break;
			}

			first = g.mFirst;
			last  = (g.mLast == -1) ? first : g.mLast;

			if (track == first)
			{
				first ++;
			}

			if (track == last)
			{
				last --;
			}

			if (last == first)
			{
				g.mFirst = first;
				g.mLast  = -1;
				changed  = true;
			}
			else if(last > first)
			{
				g.mFirst = first;
				g.mLast  = last;
				changed  = true;
			}
			break;
		}
	}

	if (changed && (sanityCheck(tmpGrps) == 0))
	{
		mGroups = tmpGrps;
		return 0;
	}

	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::delGroup(int gid)
{
	int ret = -1;

	Groups_t::const_iterator cit;

	for (cit = mGroups.cbegin(); cit != mGroups.cend(); cit ++)
	{
		if (cit->mGid == gid)
		{
			mGroups.erase(cit);
			ret = 0;
			break;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::setDiscTitle(const std::string& title)
{
	mGroups.at(0).mName = title;
	return 0;
}

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::renameGroup(int gid, const std::string& title)
{
	for (auto& g : mGroups)
	{
		if (g.mGid == gid)
		{
			g.mName = title;
			return 0;
		}
	}

	return -1;
}

//------------------------------------------------------------------------------
//! @brief      Creates a md header.
//!
//! @param[in]  content  The content
//!
//! @return     The handle md header.
//------------------------------------------------------------------------------
HndMdHdr create_md_header(const char* content)
{
	return static_cast<HndMdHdr>(new CMDiscHeader(content));
}

//------------------------------------------------------------------------------
//! @brief         free the MD header
//!
//! @param[in/out] hdl   handle to MD header
//------------------------------------------------------------------------------
void free_md_header(HndMdHdr* hdl)
{
	if (*hdl != nullptr)
	{
		delete static_cast<CMDiscHeader*>(*hdl);
		*hdl = nullptr;
	}
}

//------------------------------------------------------------------------------
//! @brief      create C string from MD header
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     c String or NULL
//------------------------------------------------------------------------------
const char* md_header_to_string(HndMdHdr hdl)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->toString().c_str();
	}
	return nullptr;
}

//------------------------------------------------------------------------------
//! @brief      add a group to the MD header
//!
//! @param[in]  hdl    The MD header handlehdl
//! @param[in]  name   The name
//! @param[in]  first  The first
//! @param[in]  last   The last
//!
//! @return     >-1 -> group id; else -> error
//------------------------------------------------------------------------------
int md_header_add_group(HndMdHdr hdl, const char* name, int16_t first, int16_t last)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->addGroup(name, first, last);
	}
	return -1;
}


//------------------------------------------------------------------------------
//! @brief      list groups in MD header
//!
//! @param[in]  hdl   The MD header handle
//------------------------------------------------------------------------------
void md_header_list_groups(HndMdHdr hdl)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		pMDH->listGroups();
	}
}

//------------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_add_track_to_group(HndMdHdr hdl, int gid, int16_t track)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->addTrackToGroup(gid, track);
	}
	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track_from_group(HndMdHdr hdl, int gid, int16_t track)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->delTrackFromGroup(gid, track);
	}
	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  hdl   The MD header handle
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_group(HndMdHdr hdl, int gid)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->delGroup(gid);
	}
	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_set_disc_title(HndMdHdr hdl, const char* title)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->setDiscTitle(title);
	}
	return -1;
}

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_rename_group(HndMdHdr hdl, int gid, const char* title)
{
	CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
	if (pMDH != nullptr)
	{
		return pMDH->renameGroup(gid, title);
	}
	return -1;
}
