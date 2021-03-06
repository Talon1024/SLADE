
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTexture.cpp
// Description: C(omposite)Texture class, represents a composite texture as
//              described in TEXTUREx entries
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "CTexture.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/SImage/SImage.h"
#include "TextureXList.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// CTPatch Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTPatch class default constructor
// -----------------------------------------------------------------------------
CTPatch::CTPatch()
{
	offset_x_ = 0;
	offset_y_ = 0;
}

// -----------------------------------------------------------------------------
// CTPatch class constructor w/initial values
// -----------------------------------------------------------------------------
CTPatch::CTPatch(string name, int16_t offset_x, int16_t offset_y)
{
	name_     = name;
	offset_x_ = offset_x;
	offset_y_ = offset_y;
}

// -----------------------------------------------------------------------------
// CTPatch class constructor copying another CTPatch
// -----------------------------------------------------------------------------
CTPatch::CTPatch(CTPatch* copy)
{
	if (copy)
	{
		name_     = copy->name_;
		offset_x_ = copy->offset_x_;
		offset_y_ = copy->offset_y_;
	}
}

// -----------------------------------------------------------------------------
// CTPatch class destructor
// -----------------------------------------------------------------------------
CTPatch::~CTPatch() {}

// -----------------------------------------------------------------------------
// Returns the entry (if any) associated with this patch via the resource
// manager. Entries in [parent] will be prioritised over entries in any other
// open archive
// -----------------------------------------------------------------------------
ArchiveEntry* CTPatch::getPatchEntry(Archive* parent)
{
	// Default patches should be in patches namespace
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name_, "patches", parent);

	// Not found in patches, check in graphics namespace
	if (!entry)
		entry = theResourceManager->getPatchEntry(name_, "graphics", parent);

	// Not found in patches, check in stand-alone texture namespace
	if (!entry)
		entry = theResourceManager->getPatchEntry(name_, "textures", parent);

	return entry;
}


// -----------------------------------------------------------------------------
//
// CTPatchEx Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTPatchEx class default constructor
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx()
{
	flip_x_      = false;
	flip_y_      = false;
	use_offsets_ = false;
	rotation_    = 0;
	alpha_       = 1.0f;
	style_       = "Copy";
	blendtype_   = 0;
	type_        = Type::Patch;
}

// -----------------------------------------------------------------------------
// CTPatchEx class constructor w/basic initial values
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(string name, int16_t offset_x, int16_t offset_y, Type type) : CTPatch(name, offset_x, offset_y)
{
	flip_x_      = false;
	flip_y_      = false;
	use_offsets_ = false;
	rotation_    = 0;
	alpha_       = 1.0f;
	style_       = "Copy";
	blendtype_   = 0;
	type_        = type;
}

// -----------------------------------------------------------------------------
// CTPatchEx class constructor copying a regular CTPatch
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(CTPatch* copy)
{
	if (copy)
	{
		flip_x_      = false;
		flip_y_      = false;
		use_offsets_ = false;
		rotation_    = 0;
		alpha_       = 1.0f;
		style_       = "Copy";
		blendtype_   = 0;
		offset_x_    = copy->xOffset();
		offset_y_    = copy->yOffset();
		name_        = copy->getName();
		type_        = Type::Patch;
	}
}

// -----------------------------------------------------------------------------
// CTPatchEx class constructor copying another CTPatchEx
// -----------------------------------------------------------------------------
CTPatchEx::CTPatchEx(CTPatchEx* copy)
{
	if (copy)
	{
		flip_x_      = copy->flip_x_;
		flip_y_      = copy->flip_y_;
		use_offsets_ = copy->useOffsets();
		rotation_    = copy->rotation_;
		alpha_       = copy->alpha_;
		style_       = copy->style_;
		blendtype_   = copy->blendtype_;
		colour_      = copy->colour_;
		offset_x_    = copy->offset_x_;
		offset_y_    = copy->offset_y_;
		name_        = copy->name_;
		type_        = copy->type_;
		translation_.copy(copy->translation_);
	}
}

// -----------------------------------------------------------------------------
// CTPatchEx class destructor
// -----------------------------------------------------------------------------
CTPatchEx::~CTPatchEx() {}

// -----------------------------------------------------------------------------
// Returns the entry (if any) associated with this patch via the resource
// manager. Entries in [parent] will be prioritised over entries in any other
// open archive
// -----------------------------------------------------------------------------
ArchiveEntry* CTPatchEx::getPatchEntry(Archive* parent)
{
	// 'Patch' type: patches > graphics
	if (type_ == Type::Patch)
	{
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name_, "patches", parent);
		if (!entry)
			entry = theResourceManager->getFlatEntry(name_, parent);
		if (!entry)
			entry = theResourceManager->getPatchEntry(name_, "graphics", parent);
		return entry;
	}

	// 'Graphic' type: graphics > patches
	if (type_ == Type::Graphic)
	{
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name_, "graphics", parent);
		if (!entry)
			entry = theResourceManager->getPatchEntry(name_, "patches", parent);
		if (!entry)
			entry = theResourceManager->getFlatEntry(name_, parent);
		return entry;
	}
	// Silence warnings
	return nullptr;
}

// -----------------------------------------------------------------------------
// Parses a ZDoom TEXTURES format patch definition
// -----------------------------------------------------------------------------
bool CTPatchEx::parse(Tokenizer& tz, Type type)
{
	// Read basic info
	type_ = type;
	name_ = tz.next().text.Upper();
	tz.adv(); // Skip ,
	offset_x_ = tz.next().asInt();
	tz.adv(); // Skip ,
	offset_y_ = tz.next().asInt();

	// Check if there is any extended info
	if (tz.advIfNext("{", 2))
	{
		// Parse extended info
		while (!tz.checkOrEnd("}"))
		{
			// FlipX
			if (tz.checkNC("FlipX"))
				flip_x_ = true;

			// FlipY
			if (tz.checkNC("FlipY"))
				flip_y_ = true;

			// UseOffsets
			if (tz.checkNC("UseOffsets"))
				use_offsets_ = true;

			// Rotate
			if (tz.checkNC("Rotate"))
				rotation_ = tz.next().asInt();

			// Translation
			if (tz.checkNC("Translation"))
			{
				// Build translation string
				string translate;
				string temp = tz.next().text;
				if (temp.Contains("="))
					temp = S_FMT("\"%s\"", temp);
				translate += temp;
				while (tz.checkNext(","))
				{
					translate += tz.next().text; // add ','
					temp = tz.next().text;
					if (temp.Contains("="))
						temp = S_FMT("\"%s\"", temp);
					translate += temp;
				}
				// Parse whole string
				translation_.parse(translate);
				blendtype_ = 1;
			}

			// Blend
			if (tz.checkNC("Blend"))
			{
				double   val;
				wxColour col;
				blendtype_ = 2;

				// Read first value
				string first = tz.next().text;

				// If no second value, it's just a colour string
				if (!tz.checkNext(","))
				{
					col.Set(first);
					colour_.set(COLWX(col));
				}
				else
				{
					// Second value could be alpha or green
					tz.adv(); // Skip ,
					double second = tz.next().asFloat();

					// If no third value, it's an alpha value
					if (!tz.checkNext(","))
					{
						col.Set(first);
						colour_.set(COLWX(col), second * 255);
						blendtype_ = 3;
					}
					else
					{
						// Third value exists, must be R,G,B,A format
						// RGB are ints in the 0-255 range; A is float in the 0.0-1.0 range
						tz.adv(); // Skip ,
						first.ToDouble(&val);
						colour_.r = val;
						colour_.g = second;
						colour_.b = tz.next().asInt();
						if (!tz.checkNext(","))
						{
							Log::error(S_FMT("Invalid TEXTURES definition, expected ',', got '%s'", tz.peek().text));
							return false;
						}
						tz.adv(); // Skip ,
						colour_.a  = tz.next().asFloat() * 255;
						blendtype_ = 3;
					}
				}
			}

			// Alpha
			if (tz.checkNC("Alpha"))
				alpha_ = tz.next().asFloat();

			// Style
			if (tz.checkNC("Style"))
				style_ = tz.next().text;

			// Read next property name
			tz.adv();
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns a text representation of the patch in ZDoom TEXTURES format
// -----------------------------------------------------------------------------
string CTPatchEx::asText()
{
	// Init text string
	string typestring = "Patch";
	if (type_ == Type::Graphic)
		typestring = "Graphic";
	string text = S_FMT("\t%s \"%s\", %d, %d\n", typestring, name_, offset_x_, offset_y_);

	// Check if we need to write any extra properties
	if (!flip_x_ && !flip_y_ && !use_offsets_ && rotation_ == 0 && blendtype_ == 0 && alpha_ == 1.0f
		&& S_CMPNOCASE(style_, "Copy"))
		return text;
	else
		text += "\t{\n";

	// Write patch properties
	if (flip_x_)
		text += "\t\tFlipX\n";
	if (flip_y_)
		text += "\t\tFlipY\n";
	if (use_offsets_)
		text += "\t\tUseOffsets\n";
	if (rotation_ != 0)
		text += S_FMT("\t\tRotate %d\n", rotation_);
	if (blendtype_ == 1 && !translation_.isEmpty())
	{
		text += "\t\tTranslation ";
		text += translation_.asText();
		text += "\n";
	}
	if (blendtype_ >= 2)
	{
		wxColour col(colour_.r, colour_.g, colour_.b);
		text += S_FMT("\t\tBlend \"%s\"", col.GetAsString(wxC2S_HTML_SYNTAX));

		if (blendtype_ == 3)
			text += S_FMT(", %1.1f\n", (double)colour_.a / 255.0);
		else
			text += "\n";
	}
	if (alpha_ < 1.0f)
		text += S_FMT("\t\tAlpha %1.2f\n", alpha_);
	if (!(S_CMPNOCASE(style_, "Copy")))
		text += S_FMT("\t\tStyle %s\n", style_);

	// Write ending
	text += "\t}\n";

	return text;
}


// -----------------------------------------------------------------------------
//
// CTexture Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTexture class constructor
// -----------------------------------------------------------------------------
CTexture::CTexture(bool extended)
{
	width_         = 0;
	height_        = 0;
	def_width_     = 0;
	def_height_    = 0;
	name_          = "";
	scale_x_       = 1.0;
	scale_y_       = 1.0;
	world_panning_ = false;
	extended_      = extended;
	defined_       = false;
	optional_      = false;
	no_decals_     = false;
	null_texture_  = false;
	offset_x_      = 0;
	offset_y_      = 0;
	type_          = "Texture";
	state_         = 0;
	in_list_       = nullptr;
	index_         = -1;
}

// -----------------------------------------------------------------------------
// CTexture class destructor
// -----------------------------------------------------------------------------
CTexture::~CTexture()
{
	for (unsigned a = 0; a < patches_.size(); a++)
		delete patches_[a];
}

// -----------------------------------------------------------------------------
// Copies the texture [tex] to this texture.
// If [keep_type] is true, the current texture type (extended/regular) will be
// kept, otherwise it will be converted to the type of [tex]
// -----------------------------------------------------------------------------
void CTexture::copyTexture(CTexture* tex, bool keep_type)
{
	// Check texture was given
	if (!tex)
		return;

	// Clear current texture
	clear();

	// Copy texture info
	name_          = tex->name_;
	width_         = tex->width_;
	height_        = tex->height_;
	def_width_     = tex->def_width_;
	def_height_    = tex->def_height_;
	scale_x_       = tex->scale_x_;
	scale_y_       = tex->scale_y_;
	world_panning_ = tex->world_panning_;
	if (!keep_type)
	{
		extended_ = tex->extended_;
		defined_  = tex->defined_;
	}
	optional_     = tex->optional_;
	no_decals_    = tex->no_decals_;
	null_texture_ = tex->null_texture_;
	offset_x_     = tex->offset_x_;
	offset_y_     = tex->offset_y_;
	type_         = tex->type_;

	// Update scaling
	if (extended_)
	{
		if (scale_x_ == 0)
			scale_x_ = 1;
		if (scale_y_ == 0)
			scale_y_ = 1;
	}
	else if (!extended_ && tex->extended_)
	{
		if (scale_x_ == 1)
			scale_x_ = 0;
		if (scale_y_ == 1)
			scale_y_ = 0;
	}

	// Copy patches
	for (unsigned a = 0; a < tex->nPatches(); a++)
	{
		CTPatch* patch = tex->getPatch(a);

		if (extended_)
		{
			if (tex->extended_)
				patches_.push_back(new CTPatchEx((CTPatchEx*)patch));
			else
				patches_.push_back(new CTPatchEx(patch));
		}
		else
			addPatch(patch->getName(), patch->xOffset(), patch->yOffset());
	}
}

// -----------------------------------------------------------------------------
// Returns the patch at [index], or NULL if [index] is out of bounds
// -----------------------------------------------------------------------------
CTPatch* CTexture::getPatch(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return nullptr;

	// Return patch at index
	return patches_[index];
}

// -----------------------------------------------------------------------------
// Returns the index of this texture within its parent list
// -----------------------------------------------------------------------------
int CTexture::getIndex()
{
	// Check if a parent TextureXList exists
	if (!in_list_)
		return index_;

	// Find this texture in the parent list
	return in_list_->textureIndex(this->getName());
}

// -----------------------------------------------------------------------------
// Clears all texture data
// -----------------------------------------------------------------------------
void CTexture::clear()
{
	name_          = "";
	width_         = 0;
	height_        = 0;
	def_width_     = 0;
	def_height_    = 0;
	scale_x_       = 1.0;
	scale_y_       = 1.0;
	defined_       = false;
	world_panning_ = false;
	optional_      = false;
	no_decals_     = false;
	null_texture_  = false;
	offset_x_      = 0;
	offset_y_      = 0;

	// Clear patches
	patches_.clear();
	for (unsigned a = 0; a < patches_.size(); a++)
		delete patches_[a];
}

// -----------------------------------------------------------------------------
// Adds a patch to the texture with the given attributes, at [index].
// If [index] is -1, the patch is added to the end of the list.
// -----------------------------------------------------------------------------
bool CTexture::addPatch(string patch, int16_t offset_x, int16_t offset_y, int index)
{
	// Create new patch
	CTPatch* np;
	if (extended_)
		np = new CTPatchEx(patch, offset_x, offset_y);
	else
		np = new CTPatch(patch, offset_x, offset_y);

	// Add it either after [index] or at the end
	if (index >= 0 && (unsigned)index < patches_.size())
		patches_.insert(patches_.begin() + index, np);
	else
		patches_.push_back(np);

	// Cannot be a simple define anymore
	this->defined_ = false;

	// Announce
	announce("patches_modified");

	return true;
}

// -----------------------------------------------------------------------------
// Removes the patch at [index].
// Returns false if [index] is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::removePatch(size_t index)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Remove the patch
	delete patches_[index];
	patches_.erase(patches_.begin() + index);

	// Cannot be a simple define anymore
	this->defined_ = false;

	// Announce
	announce("patches_modified");

	return true;
}

// -----------------------------------------------------------------------------
// Removes all instances of [patch] from the texture.
// Returns true if any were removed, false otherwise
// -----------------------------------------------------------------------------
bool CTexture::removePatch(string patch)
{
	// Go through patches
	bool removed = false;
	for (unsigned a = 0; a < patches_.size(); a++)
	{
		if (S_CMP(patches_[a]->getName(), patch))
		{
			delete patches_[a];
			patches_.erase(patches_.begin() + a);
			removed = true;
			a--;
		}
	}

	// Cannot be a simple define anymore
	this->defined_ = false;

	if (removed)
		announce("patches_modified");

	return removed;
}

// -----------------------------------------------------------------------------
// Replaces the patch at [index] with [newpatch], and updates its associated
// ArchiveEntry with [newentry].
// Returns false if [index] is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::replacePatch(size_t index, string newpatch)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Replace patch at [index] with new
	patches_[index]->setName(newpatch);

	// Announce
	announce("patches_modified");

	return true;
}

// -----------------------------------------------------------------------------
// Duplicates the patch at [index], placing the duplicated patch at
// [offset_x],[offset_y] from the original.
// Returns false if [index] is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::duplicatePatch(size_t index, int16_t offset_x, int16_t offset_y)
{
	// Check index
	if (index >= patches_.size())
		return false;

	// Get patch info
	CTPatch* dp = patches_[index];

	// Add duplicate patch
	if (extended_)
		patches_.insert(patches_.begin() + index, new CTPatchEx((CTPatchEx*)patches_[index]));
	else
		patches_.insert(patches_.begin() + index, new CTPatch(patches_[index]));

	// Offset patch by given amount
	patches_[index + 1]->setOffsetX(dp->xOffset() + offset_x);
	patches_[index + 1]->setOffsetY(dp->yOffset() + offset_y);

	// Cannot be a simple define anymore
	this->defined_ = false;

	// Announce
	announce("patches_modified");

	return true;
}

// -----------------------------------------------------------------------------
// Swaps the patches at [p1] and [p2].
// Returns false if either index is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTexture::swapPatches(size_t p1, size_t p2)
{
	// Check patch indices are correct
	if (p1 >= patches_.size() || p2 >= patches_.size())
		return false;

	// Swap the patches
	CTPatch* temp = patches_[p1];
	patches_[p1]  = patches_[p2];
	patches_[p2]  = temp;

	// Announce
	announce("patches_modified");

	return true;
}

// -----------------------------------------------------------------------------
// Parses a TEXTURES format texture definition
// -----------------------------------------------------------------------------
bool CTexture::parse(Tokenizer& tz, string type)
{
	// Check if optional
	if (tz.advIfNext("optional"))
		optional_ = true;

	// Read basic info
	type_     = type;
	extended_ = true;
	defined_  = false;
	name_     = tz.next().text.Upper();
	tz.adv(); // Skip ,
	width_ = tz.next().asInt();
	tz.adv(); // Skip ,
	height_ = tz.next().asInt();

	// Check for extended info
	if (tz.advIfNext("{", 2))
	{
		// Read properties
		while (!tz.check("}"))
		{
			// Check if end of text is reached (error)
			if (tz.atEnd())
			{
				Log::error(S_FMT("Error parsing texture %s: End of text found, missing } perhaps?", name_));
				return false;
			}

			// XScale
			if (tz.checkNC("XScale"))
				scale_x_ = tz.next().asFloat();

			// YScale
			else if (tz.checkNC("YScale"))
				scale_y_ = tz.next().asFloat();

			// Offset
			else if (tz.checkNC("Offset"))
			{
				offset_x_ = tz.next().asInt();
				tz.skipToken(); // Skip ,
				offset_y_ = tz.next().asInt();
			}

			// WorldPanning
			else if (tz.checkNC("WorldPanning"))
				world_panning_ = true;

			// NoDecals
			else if (tz.checkNC("NoDecals"))
				no_decals_ = true;

			// NullTexture
			else if (tz.checkNC("NullTexture"))
				null_texture_ = true;

			// Patch
			else if (tz.checkNC("Patch"))
			{
				CTPatchEx* patch = new CTPatchEx();
				patch->parse(tz);
				patches_.push_back(patch);
			}

			// Graphic
			else if (tz.checkNC("Graphic"))
			{
				CTPatchEx* patch = new CTPatchEx();
				patch->parse(tz, CTPatchEx::Type::Graphic);
				patches_.push_back(patch);
			}

			// Read next property
			tz.adv();
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Parses a HIRESTEX define block
// -----------------------------------------------------------------------------
bool CTexture::parseDefine(Tokenizer& tz)
{
	type_               = "Define";
	extended_           = true;
	defined_            = true;
	name_               = tz.next().text.Upper();
	def_width_          = tz.next().asInt();
	def_height_         = tz.next().asInt();
	width_              = def_width_;
	height_             = def_height_;
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name_);
	if (entry)
	{
		SImage image;
		if (image.open(entry->getMCData()))
		{
			width_   = image.getWidth();
			height_  = image.getHeight();
			scale_x_ = (double)width_ / (double)def_width_;
			scale_y_ = (double)height_ / (double)def_height_;
		}
	}
	CTPatchEx* patch = new CTPatchEx(name_);
	patches_.push_back(patch);
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string representation of the texture, in ZDoom TEXTURES format
// -----------------------------------------------------------------------------
string CTexture::asText()
{
	// Can't write non-extended texture as text
	if (!extended_)
		return "";

	// Define block
	if (defined_)
		return S_FMT("define \"%s\" %d %d\n", name_, def_width_, def_height_);

	// Init text string
	string text;
	if (optional_)
		text = S_FMT("%s Optional \"%s\", %d, %d\n{\n", type_, name_, width_, height_);
	else
		text = S_FMT("%s \"%s\", %d, %d\n{\n", type_, name_, width_, height_);

	// Write texture properties
	if (scale_x_ != 1.0)
		text += S_FMT("\tXScale %1.3f\n", scale_x_);
	if (scale_y_ != 1.0)
		text += S_FMT("\tYScale %1.3f\n", scale_y_);
	if (offset_x_ != 0 || offset_y_ != 0)
		text += S_FMT("\tOffset %d, %d\n", offset_x_, offset_y_);
	if (world_panning_)
		text += "\tWorldPanning\n";
	if (no_decals_)
		text += "\tNoDecals\n";
	if (null_texture_)
		text += "\tNullTexture\n";

	// Write patches
	for (unsigned a = 0; a < patches_.size(); a++)
		text += ((CTPatchEx*)patches_[a])->asText();

	// Write ending
	text += "}\n\n";

	return text;
}

// -----------------------------------------------------------------------------
// Converts the texture to 'extended' (ZDoom TEXTURES) format
// -----------------------------------------------------------------------------
bool CTexture::convertExtended()
{
	// Simple conversion system for defines
	if (defined_)
		defined_ = false;

	// Don't convert if already extended
	if (extended_)
		return true;

	// Convert scale if needed
	if (scale_x_ == 0)
		scale_x_ = 1;
	if (scale_y_ == 0)
		scale_y_ = 1;

	// Convert all patches over to extended format
	for (unsigned a = 0; a < patches_.size(); a++)
	{
		CTPatchEx* expatch = new CTPatchEx(patches_[a]);
		delete patches_[a];
		patches_[a] = expatch;
	}

	// Set extended flag
	extended_ = true;

	return true;
}

// -----------------------------------------------------------------------------
// Converts the texture to 'regular' (TEXTURE1/2) format
// -----------------------------------------------------------------------------
bool CTexture::convertRegular()
{
	// Don't convert if already regular
	if (!extended_)
		return true;

	// Convert scale
	if (scale_x_ == 1)
		scale_x_ = 0;
	else
		scale_x_ *= 8;
	if (scale_y_ == 1)
		scale_y_ = 0;
	else
		scale_y_ *= 8;

	// Convert all patches over to normal format
	for (unsigned a = 0; a < patches_.size(); a++)
	{
		CTPatch* npatch = new CTPatch(patches_[a]->getName(), patches_[a]->xOffset(), patches_[a]->yOffset());
		delete patches_[a];
		patches_[a] = npatch;
	}

	// Unset extended flag
	extended_ = false;
	defined_  = false;

	return true;
}

// -----------------------------------------------------------------------------
// Generates a SImage representation of this texture, using patches from
// [parent] primarily, and the palette [pal]
// -----------------------------------------------------------------------------
bool CTexture::toImage(SImage& image, Archive* parent, Palette* pal, bool force_rgba)
{
	// Init image
	image.clear();
	image.resize(width_, height_);

	// Add patches
	SImage         p_img(PALMASK);
	si_drawprops_t dp;
	dp.src_alpha = false;
	if (defined_)
	{
		CTPatchEx* patch = (CTPatchEx*)patches_[0];
		if (!loadPatchImage(0, p_img, parent, pal))
			return false;
		width_  = p_img.getWidth();
		height_ = p_img.getHeight();
		image.resize(width_, height_);
		scale_x_ = (double)width_ / (double)def_width_;
		scale_y_ = (double)height_ / (double)def_height_;
		image.drawImage(p_img, 0, 0, dp, pal, pal);
	}
	else if (extended_)
	{
		// Extended texture

		// Add each patch to image
		for (unsigned a = 0; a < patches_.size(); a++)
		{
			CTPatchEx* patch = (CTPatchEx*)patches_[a];

			// Load patch entry
			if (!loadPatchImage(a, p_img, parent, pal))
				continue;

			// Handle offsets
			int ofs_x = patch->xOffset();
			int ofs_y = patch->yOffset();
			if (patch->useOffsets())
			{
				ofs_x -= p_img.offset().x;
				ofs_y -= p_img.offset().y;
			}

			// Apply translation before anything in case we're forcing rgba (can't translate rgba images)
			if (patch->getBlendType() == 1)
				p_img.applyTranslation(&(patch->getTranslation()), pal, force_rgba);

			// Convert to RGBA if forced
			if (force_rgba)
				p_img.convertRGBA(pal);

			// Flip/rotate if needed
			if (patch->flipX())
				p_img.mirror(false);
			if (patch->flipY())
				p_img.mirror(true);
			if (patch->getRotation() != 0)
				p_img.rotate(patch->getRotation());

			// Setup transparency blending
			dp.blend     = NORMAL;
			dp.alpha     = 1.0f;
			dp.src_alpha = false;
			if (patch->getStyle() == "CopyAlpha" || patch->getStyle() == "Overlay")
				dp.src_alpha = true;
			else if (patch->getStyle() == "Translucent" || patch->getStyle() == "CopyNewAlpha")
				dp.alpha = patch->getAlpha();
			else if (patch->getStyle() == "Add")
			{
				dp.blend = ADD;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "Subtract")
			{
				dp.blend = SUBTRACT;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "ReverseSubtract")
			{
				dp.blend = REVERSE_SUBTRACT;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "Modulate")
			{
				dp.blend = MODULATE;
				dp.alpha = patch->getAlpha();
			}

			// Setup patch colour
			if (patch->getBlendType() == 2)
				p_img.colourise(patch->getColour(), pal);
			else if (patch->getBlendType() == 3)
				p_img.tint(patch->getColour(), patch->getColour().fa(), pal);


			// Add patch to texture image
			image.drawImage(p_img, ofs_x, ofs_y, dp, pal, pal);
		}
	}
	else
	{
		// Normal texture

		// Add each patch to image
		for (unsigned a = 0; a < patches_.size(); a++)
		{
			CTPatch* patch = patches_[a];
			if (Misc::loadImageFromEntry(&p_img, patch->getPatchEntry(parent)))
				image.drawImage(p_img, patch->xOffset(), patch->yOffset(), dp, pal, pal);
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads the image for the patch at [pindex] into [image].
// Can deal with textures-as-patches
// -----------------------------------------------------------------------------
bool CTexture::loadPatchImage(unsigned pindex, SImage& image, Archive* parent, Palette* pal)
{
	// Check patch index
	if (pindex >= patches_.size())
		return false;

	CTPatch* patch = patches_[pindex];

	// If the texture is extended, search for textures-as-patches first
	// (as long as the patch name is different from this texture's name)
	if (extended_ && !(S_CMPNOCASE(patch->getName(), name_)))
	{
		// Search the texture list we're in first
		if (in_list_)
		{
			for (unsigned a = 0; a < in_list_->nTextures(); a++)
			{
				CTexture* tex = in_list_->getTexture(a);

				// Don't look past this texture in the list
				if (tex->getName() == name_)
					break;

				// Check for name match
				if (S_CMPNOCASE(tex->getName(), patch->getName()))
				{
					// Load texture to image
					return tex->toImage(image, parent, pal);
				}
			}
		}

		// Otherwise, try the resource manager
		// TODO: Something has to be ignored here. The entire archive or just the current list?
		CTexture* tex = theResourceManager->getTexture(patch->getName(), parent);
		if (tex)
			return tex->toImage(image, parent, pal);
	}

	// Get patch entry
	ArchiveEntry* entry = patch->getPatchEntry(parent);

	// Load entry to image if valid
	if (entry)
		return Misc::loadImageFromEntry(&image, entry);

	// Maybe it's a texture?
	entry = theResourceManager->getTextureEntry(patch->getName(), "", parent);

	if (entry)
		return Misc::loadImageFromEntry(&image, entry);

	return false;
}
