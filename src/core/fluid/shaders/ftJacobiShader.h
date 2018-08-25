
#pragma once

#include "ofMain.h"
#include "ftShader.h"

namespace flowTools {
	
	class ftJacobiShader : public ftShader {
	public:
		ftJacobiShader() {
			bInitialized = true;
			if (ofIsGLProgrammableRenderer()) { glThree(); } else { glTwo(); }
			string shaderName = "ftJacobiShader";
			if (bInitialized) { ofLogVerbose(shaderName + " initialized"); }
			else { ofLogWarning(shaderName + " failed to initialize"); }
//			load("tempShader/ftVertexShader.vert", "tempShader/" + shaderName + ".frag");
		}
		
	protected:
		void glTwo() {
			fragmentShader = GLSL120(
									 uniform sampler2DRect Pressure;
									 uniform sampler2DRect Divergence;
									 uniform sampler2DRect Obstacle;
									 uniform float Alpha;
									 //	   uniform float InverseBeta = 0.25;
									 
									 void fTexNeighbors(sampler2DRect tex, vec2 st,
														out float left, out float right, out float bottom, out float top) {
										 left   = texture2DRect(tex, st - vec2(1, 0)).x;
										 right  = texture2DRect(tex, st + vec2(1, 0)).x;
										 bottom = texture2DRect(tex, st - vec2(0, 1)).x;
										 top    = texture2DRect(tex, st + vec2(0, 1)).x;
									 }
									 
									 void fRoundTexNeighbors(sampler2DRect tex, vec2 st,
															 out float left, out float right, out float bottom, out float top) {
										 left   = ceil(texture2DRect(tex, st - vec2(1, 0)).x - 0.5); // round not available
										 right  = ceil(texture2DRect(tex, st + vec2(1, 0)).x - 0.5);
										 bottom = ceil(texture2DRect(tex, st - vec2(0, 1)).x - 0.5);
										 top    = ceil(texture2DRect(tex, st + vec2(0, 1)).x - 0.5);
									 }
									 
									 void main() {
										 
										 vec2 st = gl_TexCoord[0].st;
										 
										 float pL; float pR; float pB; float pT;
										 fTexNeighbors (Pressure, st, pL, pR, pB, pT);
										 float pC = texture2DRect(Pressure, st).x;
										 
										 float oL; float oR; float oB; float oT;
										 fRoundTexNeighbors (Obstacle, st, oL, oR, oB, oT);
										 
										 float bC = texture2DRect(Divergence, st ).x;
										 
										 //   if (oL > 0.9) pL = pC;
										 //   if (oR > 0.9) pR = pC;
										 //   if (oB > 0.9) pB = pC;
										 //   if (oT > 0.9) pT = pC;
										 
										 pL = pL * (1.0 - oL) + pC * oL;
										 pR = pR * (1.0 - oR) + pC * oR;
										 pB = pB * (1.0 - oB) + pC * oB;
										 pT = pT * (1.0 - oT) + pC * oT;
										 
										 gl_FragColor = vec4((pL + pR + pB + pT + Alpha * bC) * 0.25, 0.0,0.0,0.0);
									 }
									 );
			
			bInitialized *= setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
			bInitialized *= linkProgram();
		}
		
		void glThree() {
			fragmentShader = GLSL150(
									 uniform sampler2DRect Pressure;
									 uniform sampler2DRect Divergence;
									 
									 in vec2 texCoordVarying;
									 out vec4 fragColor;
									 
									 void main() {
										 vec2 st = texCoordVarying;
										 float pL = texture(Pressure, st - vec2(1, 0)).x;
										 float pR = texture(Pressure, st + vec2(1, 0)).x;
										 float pB = texture(Pressure, st - vec2(0, 1)).x;
										 float pT = texture(Pressure, st + vec2(0, 1)).x;
										 float div = texture(Divergence, st ).x;
										 float alpha = -1;
										 float beta = 0.25;
										 float pres = (pL + pR + pB + pT + alpha * div) * beta;
										 fragColor = vec4(pres, 0.0, 0.0, 0.0);
									 }
									 );
			
			bInitialized *= setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
			bInitialized *= setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShader);
			bInitialized *= bindDefaults();
			bInitialized *= linkProgram();
		}
		
	public:
		void update(ofFbo& _fbo, ofTexture& _backTex, ofTexture& _divergenceTexture, ofTexture& _obsTex, float _cellSize){
			_fbo.begin();
			begin();
			setUniform1f("Alpha", -_cellSize * _cellSize);
			//			setUniform1f("InverseBeta", _inverseBeta);
			setUniformTexture("Pressure", _backTex, 0);
			setUniformTexture("Divergence", _divergenceTexture, 1);
			setUniformTexture("Obstacle", _obsTex, 2);
			renderFrame(_fbo.getWidth(), _fbo.getHeight());
			end();
			_fbo.end();
		}
	};
}

