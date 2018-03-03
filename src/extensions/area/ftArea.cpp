#include "ftArea.h"

namespace flowTools {
	
	void ftArea::setup(int _width, int _height, string _name) {
		
		numChannels = 0;
		bAllocated = false;
	
		width = _width;
		height = _height;
//		scaleFbo.allocate(width, height); // allocate 
//		allocate(scaleFbo.getTexture());
		
		numPixels = _width * _height;
		roi = ofRectangle(0,0,1,1);
		
		meanMagnitude = 0;
		stdevMagnitude = 0;
		magnitudes.resize(numPixels, 0);
		
		parameters.setName("area " + _name);
		parameters.add(pMeanMagnitude.set("mean mag", 0, 0, 1));
		parameters.add(pStdevMagnitude.set("stdev mag", 0, 0, 1));
		pMeanMagnitude.addListener(this, &ftArea::pFloatListener);
		pStdevMagnitude.addListener(this, &ftArea::pFloatListener);
		pVelocity.resize(4);
		for (int i=0; i<4; i++) {
			parameters.add(pVelocity[i].set("velocity " + ofToString(i), 0, -1, 1));
			pVelocity[i].addListener(this, &ftArea::pFloatListener);
		}
		
		roiParameters.setName("ROI " + _name);
		pRoi.resize(4);
		roiParameters.add(pRoi[0].set("x", 0, 0, 1));
		roiParameters.add(pRoi[1].set("y", 0, 0, 1));
		roiParameters.add(pRoi[2].set("width", 1, 0, 1));
		roiParameters.add(pRoi[3].set("height", 1, 0, 1));
		for (int i=0; i<4; i++) {
			pRoi[i].addListener(this, &ftArea::pFloatListener);
			pRoi[i].addListener(this, &ftArea::pRoiListener);
		}
		parameters.add(roiParameters);
	}
	
	void ftArea::update(ofTexture& _texture) {
		if (!bAllocated || ftUtil::getInternalFormat(_texture) != ftUtil::getInternalFormat(scaleFbo)) { allocate(_texture); }
		
		ofPushStyle();
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		ftUtil::black(scaleFbo);
		ftUtil::roi(scaleFbo, _texture, roi);
		ofPopStyle();
		
		ftUtil::toPixels(scaleFbo, pixels);
		float* floatPixelData = pixels.getData();
		
		vector<float> totalVelocity;
		totalVelocity.resize(numChannels, 0);
		for (int i=0; i<numPixels; i++) {
			float mag = 0;
			for (int j=0; j<numChannels; j++) {
				float vel = floatPixelData[i * numChannels + j] * 10.0 ; // times 10 should give values closer to 1
				totalVelocity[j] += vel;
				mag += vel * vel;
			}
			magnitudes[i] = sqrt(mag);
		}
		getMeanStDev(magnitudes, meanMagnitude, stdevMagnitude);
		
		float length;
		for (int i=0; i<numChannels; i++) {
			length += totalVelocity[i] * totalVelocity[i];
		}
		length = sqrt(length);
		for (int i=0; i<numChannels; i++) {
			direction[i] = totalVelocity[i] / length; // normalized velocity
			velocity[i] = direction[i] * meanMagnitude;
		}
		
		for (int i=0; i<numChannels; i++) {
			pVelocity[i] = velocity[i];
		}
		
		for (int i=numChannels; i<pVelocity.size(); i++) {
			pVelocity[i] = 0;
		}
		
		pMeanMagnitude.set(meanMagnitude);
		pStdevMagnitude.set(stdevMagnitude);
	}
	
	void ftArea::allocate(ofTexture &_tex) {
		ofTextureData& texData = _tex.getTextureData();
		
		cout << "allocate" << endl;
		
		internalFormat = texData.glInternalFormat;
		bAllocated = true;
		
		switch(internalFormat){
			case GL_R32F: 		numChannels = 1; break;
			case GL_RG32F: 		numChannels = 2; break;
			case GL_RGB32F: 	numChannels = 3; break;
			case GL_RGBA32F:	numChannels = 4; break;
			default:
				numChannels = 0;
				bAllocated = false;
				ofLogWarning("ftArea") << "allocate: " << "ftArea works with float textures only";
				return;
		}
		
		scaleFbo.allocate(width, height, internalFormat);
		ftUtil::black(scaleFbo);
		
		direction.clear();
		direction.resize(numChannels, 0);
		velocity.clear();
		velocity.resize(numChannels, 0);
		
		pixels.allocate(width, height, numChannels);
	}
	
	void ftArea::setRoi(ofRectangle _rect) {
		float x = _rect.x;
		float y = _rect.y;
		float maxW = 1.0 - x;
		float maxH = 1.0 - y;
		float w = min(_rect.width, maxW);
		float h = min(_rect.height, maxH);
		
		roi = ofRectangle(x, y, w, h);
		
		if (pRoi[0] != x) { pRoi[0].set(x); }
		if (pRoi[1] != y) { pRoi[1].set(y); }
		if (pRoi[2].getMax() != maxW) { pRoi[2].setMax(maxW); pRoi[2].set(w); }
		if (pRoi[3].getMax() != maxH) { pRoi[3].setMax(maxH); pRoi[3].set(h); }
		if (pRoi[2] != w) { pRoi[2].set(w); }
		if (pRoi[3] != h) { pRoi[3].set(h); }
	}
	
	void ftArea::getMeanStDev(vector<float> &_v, float &_mean, float &_stDev) {
		float mean = accumulate(_v.begin(), _v.end(), 0.0) / (float)_v.size();
		std::vector<float> diff(_v.size());
		std::transform(_v.begin(), _v.end(), diff.begin(), std::bind2nd(std::minus<float>(), mean));
		float sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
		float stDev = std::sqrt(sq_sum / _v.size());
		
		_mean = mean;
		_stDev = stDev;
	}
}