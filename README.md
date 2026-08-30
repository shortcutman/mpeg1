# danpg1

MPEG1 video and audio (layer 2) decoder.

Written for fun in C++ with the only dependencies being GoogleTest, SDL3, and ImGui. No dependencies used for any actual decoding. Implemented from the original specifications, were problems were encountered I reviewed other open source implementations for it. [jsmpeg](https://github.com/phoboslab/jsmpeg) was an original inspiration and a great resource where I encountered issues. A lot of code was leveraged from my jpeg decoder implementation I did a while back.