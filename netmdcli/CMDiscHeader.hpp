#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

//------------------------------------------------------------------------------
//! @brief      This class describes a minidisc header
//------------------------------------------------------------------------------
class CMDiscHeader
{
	//-----------------------------------------------------------------------------
	//! @brief      track group 
	//-----------------------------------------------------------------------------
	typedef struct
	{
		int      mGid;      //!< group id
		int16_t  mFirst;	//!< first track
		int16_t  mLast;		//!< last track
		std::string mName;	//!< group name
	} Group_t;

	using Groups_t = std::vector<Group_t>;

public:
	//-----------------------------------------------------------------------------
	//! @brief      Constructs a new instance.
	//-----------------------------------------------------------------------------
	CMDiscHeader();
	
	//-----------------------------------------------------------------------------
	//! @brief      Constructs a new instance.
	//!
	//! @param[in]  header  The RAW disc header as string
	//-----------------------------------------------------------------------------
	CMDiscHeader(const std::string& header);

	//-----------------------------------------------------------------------------
	//! @brief      create header from string
	//!
	//! @param[in]  header  The RAW disc header as string
	//!
	//! @return     { description_of_the_return_value }
	//-----------------------------------------------------------------------------
	int fromString(const std::string& header);

	//-----------------------------------------------------------------------------
	//! @brief      Returns a string representation of the object.
	//!
	//! @return     String representation of the object.
	//-----------------------------------------------------------------------------
	std::string toString() const;

	//-----------------------------------------------------------------------------
	//! @brief      Adds a group.
	//!
	//! @param[in]  name   The name
	//! @param[in]  first  The first track (optional)
	//! @param[in]  last   The last track (otional)
	//!
	//! @return     >= 0 -> group idx; -1 -> error
	//-----------------------------------------------------------------------------
	int addGroup(const std::string& name, int16_t first = -1, int16_t last = -1);

	//-----------------------------------------------------------------------------
	//! @brief      list/print all groups
	//-----------------------------------------------------------------------------
	void listGroups() const;

	//-----------------------------------------------------------------------------
	//! @brief      Adds a track to group.
	//!
	//! @param[in]  gid    The group id
	//! @param[in]  track  The track number
	//!
	//! @return     0 -> ok; -1 -> error
	//-----------------------------------------------------------------------------
	int addTrackToGroup(int gid, int16_t track);

	//-----------------------------------------------------------------------------
	//! @brief      remove a track from a group.
	//!
	//! @param[in]  gid    The group id
	//! @param[in]  track  The track number
	//!
	//! @return     0 -> ok; -1 -> error
	//-----------------------------------------------------------------------------
	int delTrackFromGroup(int gid, int16_t track);

	//-----------------------------------------------------------------------------
	//! @brief      remove a group (included tracks become ungrouped)
	//!
	//! @param[in]  gid   The group id
	//!
	//! @return     0 -> ok; -1 -> error
	//-----------------------------------------------------------------------------
	int delGroup(int gid);

	//-----------------------------------------------------------------------------
	//! @brief      Sets the disc title.
	//!
	//! @param[in]  title  The title
	//!
	//! @return     0 -> ok; else -> error
	//-----------------------------------------------------------------------------
	int setDiscTitle(const std::string& title);

protected:
	//-----------------------------------------------------------------------------
	//! @brief      check groups / tracks for sanity
	//!
	//! @param[in]  grps  The grps
	//!
	//! @return     0 -> all well; -1 -> error
	//-----------------------------------------------------------------------------
	int sanityCheck(const Groups_t& grps) const;

private:
	Groups_t mGroups;
	int      mGroupId;
};