#ifndef K_CV_STRUCTUREDLIGHT_H
#define K_CV_STRUCTUREDLIGHT_H

#include "../ImageChannel.h"
#include "../filter/Interpolation.h"

#include "../../math/statistics/Median.h"
#include "../../math/statistics/Minimum.h"
#include "../../math/statistics/Maximum.h"

#include "../../math/statistics/Statistics.h"

#include <fstream>

namespace K {

	class StructuredLight {

	public:

		/**
		 * describes one dataset,
		 * containing a reference image [everything is lit]
		 * and images for several pattern [2 lines, 4, lines, 8 lines, 16, lines, ...]
		 */
		struct Dataset {

			/** the reference image */
			K::ImageChannel ref;

			/** one image per pattern */
			std::vector<K::ImageChannel> levels;

			/** ctor */
			Dataset(const K::ImageChannel& ref, const std::vector<K::ImageChannel>& levels) : ref(ref), levels(levels) {;}

		};


	private:


	//	/** convert x coordinate to a line number */
	//	std::vector<int> xToLineNr;

	//	/** convert line-number to x-coordinate */
	//	std::vector<float> lineNumberToX;

	//	/** convert line-number to width in pixels */
	//	std::vector<float> lineNumberToLineWidth;

		K::DataMatrix<int> xyToLineNr;

		K::DataMatrix<float> yAndLineNumberToX;

		K::DataMatrix<float> yAndLineNumberToLineWidth;

		/** number of patterns to use */
		//int numLevels = 8;

		const float minBrightness = 0.15f;

		const float dDarker = 0.10f;
		const float dSame = 0.03f;

		const float UNKNOWN_DEPTH = 0.0f;

		const int UNKNOWN_LINE_NUMBER = -1;

		float camToPlaneDist1 = 0;
		float camToPlaneDist2 = 0;


	public:

		/** ctor */
		StructuredLight() {
			;
		}

		/** depending on the number of patterns, the number of visible lines variies */
		int getNumLines(const int numLevels) const {
			return 1 << numLevels;
		}

		/**
		 * calibrate the estimator by using the provided calibration dataset.
		 * this dataset contains the images [with all patterns used also used in production]
		 * from a plain surface.
		 */
		void calibrate(const Dataset& ds, const float camToPlaneDist1 = 0, const float camToPlaneDist2 = 0) {

			this->camToPlaneDist1 = camToPlaneDist1;
			this->camToPlaneDist2 = camToPlaneDist2;

			const int numLevels = ds.levels.size();
			const int numLines = getNumLines(numLevels);

			// determine the line-number for every pixel in the camera image
			xyToLineNr = getLineNumbers(ds.ref, ds.levels);

			// allocate
			yAndLineNumberToX = K::DataMatrix<float>(ds.ref.getHeight(), numLines);
			yAndLineNumberToLineWidth = K::DataMatrix<float>(ds.ref.getHeight(), numLines);



	//		// determine the line-number for each (x,y)
	//		for (int y = 0; y < ds.ref.getHeight(); ++y) {
	//			for (int x = 0; x < ds.ref.getWidth(); ++x) {
	//				const int ln = lineNumbers.get(x,y);
	//				xyToLineNr.set(x,y,ln);
	//			}
	//		}

			// fix gaps (missing/undetermined/erroneous values)
			// by ensuring a constantly growing line-number over the x-range
			for (int y = 2; y < xyToLineNr.getHeight()-2; ++y) {

				for (int x = 2; x < xyToLineNr.getWidth()-2; ++x) {
					const int ln0 = xyToLineNr.get(x-1, y);
					const int ln1 = xyToLineNr.get(x,   y);
					if (ln1 < ln0) {
						xyToLineNr.set(x,y,ln0);
					}
				}

//				// make the lines more continuous (smoothen potential steps)
//				K::DataMatrix<int> copy = xyToLineNr;
//				for (int x = 2; x < copy.getWidth()-2; ++x) {
//					const int ln0a = copy.get(x-2, y);
//					const int ln0b = copy.get(x-1, y);
//					const int ln2a = copy.get(x+1, y);
//					const int ln2b = copy.get(x+2, y);
//					const int ln1 = std::round((ln0a+ln0b + ln2a+ln2b) / 4.0f);
//					xyToLineNr.set(x,y,ln1);
//				}

			}

			std::ofstream out("/tmp/ln.dat");
			for (int y = 8; y < xyToLineNr.getHeight()-8; y+=8) {
				for (int x = 0; x < xyToLineNr.getWidth(); ++x) {
					const int ln = xyToLineNr.get(x, y);
					out << x << " " << ln+y << std::endl;
				}
				out << std::endl << std::endl;
			}
			out.close();

			// build the reverse lookup: y+line-number -> x
			for (int y = 0; y < xyToLineNr.getHeight(); ++y) {
				for (int x = 0; x < xyToLineNr.getWidth(); ++x) {
					const int ln = xyToLineNr.get(x,y);
					if (ln == -1) {continue;}
					yAndLineNumberToX.set(y, ln, x);
				}
			}

			// replace the x position of each line with the line's center
			for (int y = 0; y < xyToLineNr.getHeight(); ++y) {
				for (int i = 0; i < numLines-1; ++i) {
					const float x1 = yAndLineNumberToX.get(y, i);
					const float x2 = yAndLineNumberToX.get(y, i+1);
					const float xc = (x1+x2)/2;
					yAndLineNumberToX.set(y, i, xc);					
				}
			}

			Statistics<float> stats;
			int num = 0;
			int numZero = 0;

			// determine the width for each line			
			for (int y = 0; y < xyToLineNr.getHeight(); ++y) {
				for (int i = 0; i < numLines-1; ++i) {
					float x1 = yAndLineNumberToX.get(y, i);
					float x2 = yAndLineNumberToX.get(y, i+1);
					float w = x2-x1;
					yAndLineNumberToLineWidth.set(y, i, w);
					if (w > 0) {stats.add(w);}
					if (w == 0) {++numZero;}
					++num;
				}
			}

			std::cout << stats.getStdDev() << std::endl;;

			// the number of values not correctly covered by calibration
			const float percentZero = numZero / (float) num;
			if (percentZero > 0.15) {
				std::cout << "more than 10% of all pixels are NOT covered by depth" << std::endl;
				//throw Exception("more than 10% of all pixels are NOT covered by depth");
			}


			const float medLineW = stats.getMedian();
			const float resolPercent = medLineW / (float) ds.ref.getWidth();
			if (resolPercent > 0.01) {
				std::cout << "very bad depth resolution [very wide lines]" << std::endl;
				//throw Exception("very bad depth resolution [very wide lines]");
			}
			//if (stats.getStdDev() > 15 * medLineW) {throw Exception("very sloppy line detectioN!");}



			std::cout << "zero: " << (percentZero*100) << "% med_slope: " << stats.getMedian() << " avg_slope: " << stats.getAvg() << std::endl;


		}


		K::ImageChannel getDepth(const Dataset& ds, const float divider) const {
			return getDepth(ds.ref, ds.levels, divider);
		}


		K::ImageChannel getDepth(const K::ImageChannel& ref, const std::vector<K::ImageChannel>& images, const float divider) const {

			K::DataMatrix<int> lineNumbers = getLineNumbers(ref, images);
			K::ImageChannel depth(ref.getWidth(), ref.getHeight());
			depth.ones();

			const float width = depth.getWidth();


			lineNumbers.forEach([&] (const int x, const int y, const int lineNr) {

				if (lineNr == UNKNOWN_LINE_NUMBER)	{depth.set(x,y,UNKNOWN_DEPTH); return;}
				if (lineNr > 32768)					{depth.set(x,y,UNKNOWN_DEPTH); return;}


				int _y = y;

				const float refX = yAndLineNumberToX.get(_y, lineNr);
				if (refX == 0) {return;}

				const float lineW = yAndLineNumberToLineWidth.get(_y, lineNr);
				const float lineW2 = lineW;
				if (lineW == 0) {return;}

				// round distance depending on the line-width as this is our uncertainty
				//const float dx = std::floor((refX - x) / lineW2) * lineW2;
				const float dx = (refX - x);

				const float xxx = ( (camToPlaneDist1 * (width-x)) + (camToPlaneDist2 * (x)) ) / width;

				// TODO: scaling factor
				float d = dx / divider;
				//if (d > 1.0) {d = UNKNOWN_DEPTH;}
				if		(d < 0.0)	{d = UNKNOWN_DEPTH;}		// ignore impossible values
				else				{d += xxx;}		// add camera-to-calibration-plane offset

				depth.set(x,y, d);

			});

			return depth;

		}


	private:

		/** determine the number of the line each pixel belongs to and get the result as 2D array */
		K::DataMatrix<int> getLineNumbers(const K::ImageChannel& ref, const std::vector<K::ImageChannel>& images) const {

			const int numLevels = images.size();
			K::DataMatrix<int> lineNumbers(ref.getWidth(), ref.getHeight());

			// process each pixel
			for (int y = 0; y < ref.getHeight(); ++y) {
				for (int x = 0; x < ref.getWidth(); ++x) {

					// get the pixel-brightness within the reference image
					const float refVal = ref.get(x,y);

					// reference is too dark. this will not work! -> ln = UNKNOWN_LINE_NUMBER
					if (refVal < minBrightness) {
						lineNumbers.set(x,y,UNKNOWN_LINE_NUMBER);
						continue;
					}

					// get the pixel-brightness within each layer-image and use it
					// to calculate the pixel's line-number
					int lineNr = 0;
					for (int i = 0; i < numLevels; ++i) {
						lineNr = lineNr << 1;
						const float levelVal = getBest(refVal, images[i], x,y);
						const float diff = levelVal - refVal;
						if		(diff < -dDarker)	{lineNr |= 1;}							// darker than reference
						else if	(diff > +dSame)		{lineNr = UNKNOWN_LINE_NUMBER; break;}	// brighter than reference! impossible!
						else						{;}										// as bright as reference
					}

					lineNumbers.set(x,y,lineNr);

				}
			}

			return lineNumbers;

		}


		/** find the pixel with the maximum value-difference to the reference using sub-pixel accuracy */
		inline float getBest(const float ref, const K::ImageChannel& img, const int x, const int y) const {
			return img.get(x,y); (void) ref;
		}

	//	/** find the pixel with the maximum value-difference to the reference using sub-pixel accuracy */
	//	K::Interpolation::Bilinear interpol;
	//	inline float getBest(const float ref, const K::ImageChannel& img, const int x, const int y) const {
	//		K::Minimum<float> med;
	//		for (float dx = -0.1f; dx <= +0.1f; dx += 0.05f) {
	//			const float val = interpol.get(img, x+dx, y);
	//			med.add(val);
	//		}
	//		return med.get();
	//	}

	//	/** find the pixel with the maximum value-difference to the reference using sub-pixel accuracy */
	//	K::Interpolation::Bilinear interpol;
	//	float getBest(const float ref, const K::ImageChannel& img, const int x, const int y) const {

	//		float maxDiff = -99999;
	//		float maxVal = 0;

	//		for (float dx = -0.5; dx <= +0.5; dx += 0.5) {
	//			const float x1 = x+dx;
	//			const float y1 = y;
	//			const float val = interpol.get(img, x1, y1);
	//			const float diff = std::abs(ref-val);
	//			if (diff > maxDiff) {maxDiff = diff; maxVal = val;}
	//		}

	//		return maxVal;

	//	}


	};

}

#endif // K_CV_STRUCTUREDLIGHT_H
