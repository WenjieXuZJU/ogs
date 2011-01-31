/**
 * \file VtkColorLookupTable.cpp
 * 23/04/2010 KR Initial implementation
 *
 */

#include "VtkColorLookupTable.h"

#include <cmath>
#include <Color.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(VtkColorLookupTable);
vtkCxxRevisionMacro(VtkColorLookupTable, "$Revision$");

VtkColorLookupTable::VtkColorLookupTable()
: _type(VtkColorLookupTable::LINEAR)
{
	this->SetNumberOfTableValues(256);
}

VtkColorLookupTable::~VtkColorLookupTable()
{
	for (std::map<double, unsigned char*>::const_iterator it = _dict.begin(); it != _dict.end(); ++it) delete it->second;
}


unsigned char VtkColorLookupTable::linInterpolation(unsigned char a, unsigned char b, double p) const
{
    return static_cast<unsigned char>(a * (1 - p) + b * p);
}

unsigned char VtkColorLookupTable::expInterpolation(unsigned char a, unsigned char b, double gamma, double p) const
{
	assert (gamma>0 && gamma<4);
	return static_cast<unsigned char>((b-a)*pow(p,gamma)+a);
}


void VtkColorLookupTable::Build()
{
	if (!_dict.empty())
	{
		// make sure that color map starts with the first color in the dictionary
		unsigned char* startcolor = new unsigned char[4];
		startcolor[0] = _dict.begin()->second[0]; startcolor[1] = _dict.begin()->second[1]; startcolor[2] = _dict.begin()->second[2]; startcolor[3] = _dict.begin()->second[3];
		std::pair<size_t, unsigned char*> lastValue(0, startcolor);
		size_t nextIndex(0);

		// make sure that color map ends with the last color in the dictionary
		std::map<double, unsigned char*>::iterator lastitr = _dict.end();
		--lastitr;
		if (lastitr->first != 1)
		{
			unsigned char* lastcolor = new unsigned char[4];
			for (size_t i=0; i<4; i++) lastcolor[i] = lastitr->second[i];
			_dict.insert( std::pair<double, unsigned char*>(1.0, lastcolor) );
		}

		for (std::map<double, unsigned char*>::const_iterator it = _dict.begin(); it != _dict.end(); ++it)
		{
			//unsigned char rgba[4];
			//rgba[0] = (*it->second)[0]; rgba[1] = (*it->second)[1]; rgba[2] = (*it->second)[2]; rgba[3] = 255;

			nextIndex = static_cast<size_t>( floor(it->first * this->GetNumberOfTableValues()) );
			if (nextIndex >= static_cast<size_t>(this->GetNumberOfTableValues())) nextIndex--; // this happens for the very last colour
			this->SetTableValue(nextIndex, it->second);

			if ( nextIndex-lastValue.first > 1 )
			{
				for (size_t i = lastValue.first+1; i < nextIndex; i++)
				{
					unsigned char int_rgba[4];
					int_rgba[3] = 255;
					double pos = (i - lastValue.first) / (static_cast<double>(nextIndex - lastValue.first));

					if (_type == VtkColorLookupTable::LINEAR)
					{
						for (size_t j=0; j<3; j++)
							int_rgba[j] = linInterpolation((lastValue.second)[j], (it->second)[j], pos);
					}
					else if (_type == VtkColorLookupTable::EXPONENTIAL)
					{
						for (size_t j=0; j<3; j++)
							int_rgba[j] = expInterpolation((lastValue.second)[j], (it->second)[j], 0.2, pos);
					}
					else	// no interpolation
					{
							for (size_t j=0; j<3; j++)
								int_rgba[j] = (lastValue.second)[j];
					}

					this->SetTableValue(i, int_rgba);
				}
			}

			lastValue.first = nextIndex;
			lastValue.second = it->second;
		}
	}
	else
		vtkLookupTable::Build();
}

void VtkColorLookupTable::readFromFile(const std::string &filename)
{
	std::map<std::string, GEOLIB::Color*> colors;
	GEOLIB::readColorLookupTable(colors, filename);
	this->SetNumberOfTableValues(colors.size());

	for (std::map<std::string, GEOLIB::Color*>::iterator it = colors.begin(); it != colors.end(); ++it)
	{
		this->SetTableValue( strtod( it->first.c_str(), 0 ), (*(it->second))[0], (*(it->second))[1], (*(it->second))[2], 255 );
	}
}

void VtkColorLookupTable::writeToFile(const std::string &filename)
{
	std::cout << "Writing color table to " << filename << "....";
	std::ofstream out( filename.c_str(), std::ios::out );

	size_t nColors = this->GetNumberOfTableValues();
	for (size_t i=0; i<nColors; i++)
	{
		unsigned char rgba[4];
		this->GetTableValue(i, rgba);
		out << i << "\t" << rgba[0] << "\t" <<rgba[1] << "\t" << rgba[2] << std::endl;
	}

	std::cout << " done." << std::endl;
	out.close();
}


void VtkColorLookupTable::SetTableValue(vtkIdType indx, unsigned char rgba[4])
{
	// Check the index to make sure it is valid
	if (indx < 0)
	{
		vtkErrorMacro("Can't set the table value for negative index " << indx);
		return;
	}
	if (indx >= this->NumberOfColors)
	{
		vtkErrorMacro("Index " << indx << " is greater than the number of colors " << this->NumberOfColors);
		return;
	}

	unsigned char *_rgba = this->Table->WritePointer(4*indx,4);
	for (size_t i=0; i<4; i++) _rgba[i]=rgba[i];

	this->InsertTime.Modified();
	this->Modified();
}

void VtkColorLookupTable::SetTableValue(vtkIdType indx, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  unsigned char rgba[4];
  rgba[0] = r; rgba[1] = g; rgba[2] = b; rgba[3] = a;
  this->SetTableValue(indx,rgba);
}

void VtkColorLookupTable::GetTableValue(vtkIdType indx, unsigned char rgba[4])
{
  unsigned char *_rgba;
  _rgba = this->Table->GetPointer(indx*4);
  for (size_t i=0; i<4; i++) rgba[i]=_rgba[i];
}

void VtkColorLookupTable::setColor(double pos, unsigned char rgba[4])
{
	if (pos>=0 && pos<=1)
	{
		unsigned char* dict_rgba = new unsigned char[4];
		for (size_t i=0; i<4; i++) dict_rgba[i] = rgba[i];
		_dict.insert( std::pair<double, unsigned char*>(pos, dict_rgba) );
	}
	else
		std::cout << "ColorLookupTable::setValue() - Error: pos should be in [0,1]" << std::endl;
}

void VtkColorLookupTable::getColor(vtkIdType indx, unsigned char rgba[4]) const
{
  indx = ((indx < this->TableRange[0]) ? static_cast<vtkIdType>(this->TableRange[0]) : (indx >= this->TableRange[1] ? static_cast<vtkIdType>(this->TableRange[1])-1 : indx));
  indx = static_cast<size_t>( floor( (indx - this->TableRange[0]) * (this->NumberOfColors/(this->TableRange[1]-this->TableRange[0])) ) );

  unsigned char *_rgba;
  _rgba = this->Table->GetPointer(indx*4);
  for (size_t i=0; i<4; i++) rgba[i]=_rgba[i];
}
