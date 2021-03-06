#ifndef GNUPLOTPLOTELEMENTLINES_H
#define GNUPLOTPLOTELEMENTLINES_H


#include <vector>
#include "GnuplotPlotElementRaw.h"


namespace K {

	class GnuplotPlotElementLines : public GnuplotPlotElementRaw {

	private:

		int lineWidth = 1;


	public:

		/** set the line-width to use for drawing */
		void setLineWidth(const int lineWidth) {this->lineWidth = lineWidth;}

		void addHeaderTo(std::stringstream& ss) const override {
			ss << "'-' with lines ";
			ss << attrCustom << " ";
			ss << " lw " << lineWidth;
			ss << " lc " << color;
			ss << " title '" << title << "'";
		}

		/** add a new point to output */
		void add(const GnuplotPoint2 p) {
			points.push_back(p);
		}

		/** remove all points */
		void clear() {
			points.clear();
		}

		/** remove the given index */
		void remove(const int idx) {
			points.erase(points.begin() + idx);
		}

		/** number of entries */
		size_t size() const {
			return points.size();
		}

		/** add an unnconected segment from A to B */
		void addSegment(const GnuplotPoint2 from, const GnuplotPoint2 to) {
			points.push_back(from);
			points.push_back(to);
			points.push_back(GnuplotPoint2::getEmpty());
			points.push_back(GnuplotPoint2::getEmpty());
		}


	};

}


#endif // GNUPLOTPLOTELEMENTLINES_H
