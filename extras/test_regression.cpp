/*
Comparison of sig figs in a float vs double.
Comparison of using 4th vs 3rd order polynomial.
Makes almost no difference when getting temp to two decimal places.
Testing floats vs doubles: No noticeable difference.
Testing 3rd order vs 4th order polynomial: No noticeable difference.
*/
#include <iostream>
#include <cmath>
#include <iomanip>

using namespace std;
const float A = 2.332225E+02;
const float B = -9.784173E-02;
const float C = 2.401275E-05;
const float D = -3.491355E-09;

const float qA = 2.9476119E+02;
const float qB = -2.0544626E-01;
const float qC = 9.3818106E-05;
const float qD = -2.3412916E-08;
const float qE = 2.1114968E-12;

// const double A = 2.33222507502202E+02;
// const double B = -9.78417339763722E-02;
// const double C = 2.40127508342357E-05;
// const double D = -3.49135518682689E-09;

// const double qA = 2.94761189325819E+02;
// const double qB = -2.05446256486982E-01;
// const double qC = 9.38181058287797E-05;
// const double qD = -2.3412916496839E-08;
// const double qE = 2.1114967506098E-12;

// 3rd Order Polynomial (Cubic Formula) TempF = A+Bx+C*x^2+D*x^3
float getCubicTemp(int ADC_value) {
    int x = ADC_value;
    float tempF = A+B*x+C*pow(x, 2)+D*pow(x, 3);
    return tempF;
}

// 4th Order Polynomial (Quartic Formula) TempF = A+Bx+C*x^2+D*x^3+E*x^4
float getQuarticTemp(int ADC_value) {
    int x = ADC_value;
    float tempF = qA+qB*x+qC*pow(x, 2)+qD*pow(x, 3)+qE*pow(x, 4);
    return tempF;
}

int main() {
    int start = 1700;
    int end = 2905;
    int ADC_value = start;
    
    while (ADC_value <= end) {
        cout << std::fixed << std::setprecision(1) << getCubicTemp(ADC_value) << ", " << getQuarticTemp(ADC_value) << endl;
        ADC_value++;
    }

    

  return 0;
}
