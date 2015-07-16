/************************************************************************************

Filename    :   DistortionChroma_vs.vsh

Copyright   :   Copyright 2014 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.2 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.2 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

float2   EyeToSourceUVScale;
float2   EyeToSourceUVOffset;

void main(in float2 Position    : POSITION,
          in float4 Color       : COLOR0,
          in float2 TexCoord0   : TEXCOORD0,
          in float2 TexCoord1   : TEXCOORD1,
          in float2 TexCoord2   : TEXCOORD2,
          out float4 oPosition  : SV_Position,          
          out float1 oColor     : COLOR,
          out float2 oTexCoord0 : TEXCOORD0,
          out float2 oTexCoord1 : TEXCOORD1,
          out float2 oTexCoord2 : TEXCOORD2)
{
    oPosition.x = Position.x;
    oPosition.y = Position.y;
    oPosition.z = 0.5;
    oPosition.w = 1.0;

    // Scale them into  UV lookup space
    float2 tc0scaled = EyeToSourceUVScale * TexCoord0 + EyeToSourceUVOffset;
    float2 tc1scaled = EyeToSourceUVScale * TexCoord1 + EyeToSourceUVOffset;
    float2 tc2scaled = EyeToSourceUVScale * TexCoord2 + EyeToSourceUVOffset;

    oTexCoord0  = tc0scaled;        // R sample.
    oTexCoord1  = tc1scaled;        // G sample.
    oTexCoord2  = tc2scaled;        // B sample.
    oColor      = Color.r;          // Used for vignette fade.
}

