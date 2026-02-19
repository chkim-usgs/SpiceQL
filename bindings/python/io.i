%module(package="pyspiceql") io

%{
  #include "io.h"
%}

%ignore writeCkFromBuffers;
%include "io.h"