SPA - Sequence-point analyzer
===

*What is this project about?*

SPA is a project developed to address one type of undefined behavior in C language: side effects and sequence points. It achieves its goal - detection of potentional problems of this kind - by static analysis.

*What is it good for and how can it be used?*

SPA is a Clang plugin, so you can use it while compiling your projects with Clang. SPA will check your program for the addressed type of undefined behavior automatically and warn you, thus letting you fix them instead of letting them go wild.
