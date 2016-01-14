#ifndef GNUPLOTSPLOTELEMENTPOINTS_H
#define GNUPLOTSPLOTELEMENTPOINTS_H

#include "GnuplotSplotElementRaw.h"
#include "attributes/GnuplotAttrColor.h"

namespace K {

	class GnuplotSplotElementPoints : public GnuplotSplotElementRaw, public GnuplotAttrColor {

	private:

		float pointSize = 0.2;
		int pointType = 7;


	public:

		void setPointType(const int t) {this->pointType = t;}

		void setPointSize(const float s) {this->pointSize = s;}

		void setColorHex(const std::string& hex) {this->color = "rgb '" + hex + "'";}

		void addHeaderTo(std::stringstream& ss) const override {
			ss << "'-' with points ";
			ss << " pt " << pointType;
			ss << " ps " << pointSize;
			ss << " lc " << color;
			ss << " title '" << title << "'";
		}

		/** add a new point to output */
		void add(const GnuplotPoint3 p) {
			points.push_back(p);
		}

	};

}

#endif // GNUPLOTSPLOTELEMENTPOINTS_H
