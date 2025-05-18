#pragma once
#include "mbed.h"


DigitalOut y(A0);
DigitalOut x(A1);

DigitalOut A(A2);
DigitalOut B(A3);
DigitalOut C(A4);
DigitalOut D(A5);

int ti = 1;
//int score;

void bindecoder(int val)
{
    if (val == 0)
        {
            A = false;
            B = false;
            C = false;
            D = false;
        }

    if (val == 1)
        {
            A = false;
            B = false;
            C = false;
            D = true;
        }

    if (val == 2)
        {
            A = false;
            B = false;
            C = true;
            D = false;
        }
    
    if (val == 3)
        {
            A = false;
            B = false;
            C = true;
            D = true;
        }

    if (val == 4)
        {
            A = false;
            B = true;
            C = false;
            D = false;
        }

    if (val == 5)
        {
            A = false;
            B = true;
            C = false;
            D = true;
        }

    if (val == 6)
        {
            A = false;
            B = true;
            C = true;
            D = false;
        }

    if (val == 7)
        {
            A = false;
            B = true;
            C = true;
            D = true;
        }

    if (val == 8)
        {
            A = true;
            B = false;
            C = false;
            D = false;
        }

    if (val == 9)
        {
            A = true;
            B = false;
            C = false;
            D = true;
        }

}
void ScoreVal(int score)
{
    int th,h,t,u;

    u = score%10;
    t = (score/10)%10;
    h = (score/100)%10;
    th = (score/1000);

    x = 1;
    y = 1;
    bindecoder(u);

    thread_sleep_for(ti);

    x = 0;
    y = 1;
    bindecoder(t);

    thread_sleep_for(ti);

    x = 1;
    y = 0;
    bindecoder(h);

    thread_sleep_for(ti);

    x = 0;
    y = 0;
    bindecoder(th);

    thread_sleep_for(ti);

}