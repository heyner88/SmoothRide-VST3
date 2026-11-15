#include "../Source/GainRider.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main() {
    smoothride::GainRider ride;
    ride.prepare(48000);
    smoothride::Settings s;
    ride.setSettings(s);
    std::vector<float> left(48000), right(48000);
    for(int i=0;i<48000;++i) left[i]=right[i]=(float)(0.5*std::sin(2*3.141592653589793*440.0*i/48000.0));
    ride.process(left.data(),right.data(),48000,true);
    for (int i=0;i<48000;++i) { assert(std::isfinite(left[i])); assert(std::abs(left[i])<=0.89126f); assert(left[i]==right[i]); }
    std::cout << "PASS: stereo, finite samples, sample peak ceiling\n";
    std::fill(left.begin(),left.end(),0.0f); std::fill(right.begin(),right.end(),0.0f);
    ride.process(left.data(),right.data(),48000,true);
    for(float x:left) assert(x==0.0f);
    std::cout << "PASS: silence stays silent (no noise amplification)\n";
    s.mix=0.0f; s.outputDb=0.0f; s.ceilingDb=-0.1f;
    ride.reset(); ride.setSettings(s);
    for(int i=0;i<48000;++i) left[i]=right[i]=(float)(0.1*std::sin(2*3.141592653589793*440.0*i/48000.0));
    const float original=left[100]; ride.process(left.data(),right.data(),48000,true);
    assert(std::abs(left[100]-original)<1.0e-5f);
    std::cout << "PASS: dry with unity gain is transparent\n";
}
