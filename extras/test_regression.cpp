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
const float A = -3.491355E-09;  // Ax^3
const float B = 2.401275E-05;   // Bx^2
const float C = -9.784173E-02;  // Cx
const float D = 2.332225E+02;   // D

const float qA = 2.1114968E-12;  // Ax^4
const float qB = -2.3412916E-08; // Bx^3
const float qC = 9.3818106E-05;  // Cx^2
const float qD = -2.0544626E-01; // Dx
const float qE = 2.9476119E+02;  // E



// const double A = -3.491355E-09;  // Ax^3
// const double B = 2.401275E-05;   // Bx^2
// const double C = -9.784173E-02;  // Cx
// const double D = 2.332225E+02;   // D

// const double qA = 2.1114968E-12;  // Ax^4
// const double qB = -2.3412916E-08; // Bx^3
// const double qC = 9.3818106E-05;  // Cx^2
// const double qD = -2.0544626E-01; // Dx
// const double qE = 2.9476119E+02;  // E

// 3rd Order Polynomial (Cubic Formula) TempF = Ax^3+Bx^2+Cx+D
float getCubicTemp(int ADC_value) {
    int x = ADC_value;
    float tempF = A*pow(x, 3)+B*pow(x, 2)+C*x+D;
    return tempF;
}

// 4th Order Polynomial (Quartic Formula) TempF = Ax^4+Bx^3+Cx^2+Dx+E
float getQuarticTemp(int ADC_value) {
    int x = ADC_value;
    float tempF = tempF = qA*pow(x, 4)+qB*pow(x, 3)+qC*pow(x, 2)+qD*x+qE;
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
