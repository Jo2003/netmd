#ifdef __cplusplus
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
	std::string toString();

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

	//-----------------------------------------------------------------------------
	//! @brief      rename one group
	//!
	//! @param[in]  gid    The group id
	//! @param[in]  title  The new title
	//!
	//! @return     0 -> ok; else -> error
	//-----------------------------------------------------------------------------
	int renameGroup(int gid, const std::string& title);

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
	Groups_t           mGroups;
	int                mGroupId;
	std::ostringstream mOss;
};

extern "C" {
#endif // __cplusplus 

//! define a MD Header handle
typedef void* HndMdHdr;

//------------------------------------------------------------------------------
//! @brief      Creates a md header.
//!
//! @param[in]  content  The content
//!
//! @return     The handle md header.
//------------------------------------------------------------------------------
HndMdHdr create_md_header(const char* content);

//------------------------------------------------------------------------------
//! @brief         free the MD header
//!
//! @param[in/out] hdl   handle to MD header
//------------------------------------------------------------------------------
void free_md_header(HndMdHdr* hdl);

//------------------------------------------------------------------------------
//! @brief      create C string from MD header
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     C string or NULL
//------------------------------------------------------------------------------
const char* md_header_to_string(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      add a group to the MD header
//!
//! @param[in]  hdl    The MD header handlehdl
//! @param[in]  name   The name
//! @param[in]  first  The first
//! @param[in]  last   The last
//!
//! @return     > -1 -> group id; else -> error
//------------------------------------------------------------------------------
int md_header_add_group(HndMdHdr hdl, const char* name, int16_t first, int16_t last);

//------------------------------------------------------------------------------
//! @brief      list groups in MD header
//!
//! @param[in]  hdl   The MD header handle
//------------------------------------------------------------------------------
void md_header_list_groups(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_add_track_to_group(HndMdHdr hdl, int gid, int16_t track);

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track_from_group(HndMdHdr hdl, int gid, int16_t track);

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  hdl   The MD header handle
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_group(HndMdHdr hdl, int gid);

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_set_disc_title(HndMdHdr hdl, const char* title);

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_rename_group(HndMdHdr hdl, int gid, const char* title);

#ifdef __cplusplus
}
#endif //  __cplusplus