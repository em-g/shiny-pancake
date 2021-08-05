# shiny-pancake

### Description
This C script offers the possibility of analyzing a video sequence in YUV format (based on the first two frames).

With these frames, the resulting images are divided into 16x16 blocks. The blocks in the 1st frame are examined in the 2nd frame and moved there. Subsequently, the resulting motion vectors are saved in a file. With this, a prediction image can be generated. Comparing the prediction image with the original second frame leads to a prediction error image. The MSE (Mean Squared Error) is then calculated with the prediction error image.

This script was executed with an American football sequence. Accordingly, all constants and buffers are designed for the dimensions of this file. To be able to analyze another sequence and calculate the error patterns and the MSE from it, the constants and buffers must be adjusted accordingly.


### Example

Original frames:

![](https://i.imgur.com/LS7jlUj.png) ![](https://i.imgur.com/jKqVDFz.png)

> frame 1 and frame 2 (Y component only)

Predicted frames:

![](https://i.imgur.com/s11P1DE.png) ![](https://i.imgur.com/NfGrhT6.png)

> prediction image and prediction error image (Y component only)

> [prediction image pixel - 127 = prediction error image pixel]

