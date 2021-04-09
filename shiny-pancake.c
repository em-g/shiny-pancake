//
//  shiny-pancake.c
//  VC - WS2021
//
//  Created by em-g on 30.03.2021.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// files
FILE * video_file;
FILE * vector_file;
FILE * vector_file_x;
FILE * vector_file_y;
FILE * mse_value_file;
FILE * prediction_error_img;
FILE * prediction_img;
FILE * error_text_file;

// video parameters
int width = 352;
int height = 288;

// frame sizes: 2Y (2*352*288), 2UV (2*176*144)
int size_of_2_frames = 253440;
int size_of_chrominances = 152064;

// buffers
unsigned char frame_array[253440];
int motion_vector_array_x[600];
int motion_vector_array_y[600];

//      frame buffers
unsigned char buffer_frame_1[352][288];
unsigned char buffer_frame_2[352][288];

//      prediction
unsigned char img_prediction[352][288];
unsigned char prediction_buffer[352 * 288];

//      error-image
unsigned char error_array[352][288];
unsigned char error_buffer[352 * 288];

// block parameters
struct MACRO_VARIABLES {
    int w0;
    int h0;
    int w1;
    int h1;
};

struct MSE_PARAMETERS {
    int w;
    int h;
    int w_mse;
    int h_mse;
};

struct MACRO_VARIABLES current_block = {
    0,
    0,
    0,
    0
};

struct MSE_PARAMETERS current_param = {
    0,
    0,
    0,
    0
};

struct MSE_PARAMETERS extracted_param = {
    0,
    0,
    0,
    0
};

int bl_h = 16;
int bl_w = 16;
double mse_value = 999999;

// helping fuctions

double get_mse_value_per_block(int b_w, int b_h, int search_w, int search_h) {
    double reconstruction_error_value = 0.0;
    double reconstruction_sum = 0.0;

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            reconstruction_error_value = buffer_frame_2[x + b_w][y + b_h] - buffer_frame_1[x + search_w][y + search_h];
            reconstruction_sum += (reconstruction_error_value * reconstruction_error_value);
        }
    }
    return reconstruction_sum / 256;
}

void error_while_loading_file(char filename[]) {
    fprintf(error_text_file, "Error while loading file: %s\n", filename);
    fclose(error_text_file);
}

double get_total_mse_value() {
    double reconstruction_error_value = 0.0;
    double reconstruction_sum = 0.0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            reconstruction_error_value = buffer_frame_2[x][y] - prediction_buffer[(y * 352) + x];
            reconstruction_sum += (reconstruction_error_value * reconstruction_error_value);
        }
    }
    return reconstruction_sum / (height * width);
}

void add_to_prediction_error(int value) {
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            prediction_buffer[w + (h * 352)] = img_prediction[w][h];
        }
    }
}

// boundary condition
struct MACRO_VARIABLES determine_margins(int h, int w) {
    struct MACRO_VARIABLES result = {
        0,
        0,
        0,
        0
    };
    if (h == 0 && w == 0) {
        result.w0 = bl_w;
        result.h0 = bl_h;
        result.w1 = 0;
        result.h1 = 0;
    } else if (h == 0 && w > 0 && w < 336) {
        result.w0 = w + bl_w;
        result.h0 = bl_h;
        result.w1 = w - bl_w;
        result.h1 = 0;
    } else if (h == 0 && w == 336) {
        result.w0 = w;
        result.h0 = bl_h;
        result.w1 = w - bl_w;
        result.h1 = 0;
    } else if (h > 0 && h < 272 && w == 0) {
        result.w0 = bl_w;
        result.h0 = h + bl_h;
        result.w1 = 0;
        result.h1 = h - bl_h;
    } else if (h == 272 && w == 0) {
        result.w0 = bl_w;
        result.h0 = h;
        result.w1 = 0;
        result.h1 = h - bl_h;
    } else if (h == 272 && w > 0 && w < 336) {
        result.w0 = w + bl_w;
        result.h0 = h + bl_h;
        result.w1 = w - bl_w;
        result.h1 = h - bl_h;
    } else if (h == 272 && w == 336) {
        result.w0 = w + bl_w;
        result.h0 = h + bl_h;
        result.w1 = w - bl_w;
        result.h1 = h - bl_h;
    } else if (w == 336 && h > 0 && h < 272) {
        result.w0 = w;
        result.h0 = h + bl_h;
        result.w1 = w - bl_w;
        result.h1 = h - bl_h;
    } else {
        result.w0 = w + bl_w;
        result.h0 = h + bl_h;
        result.w1 = w - bl_w;
        result.h1 = h - bl_h;
    }
    return result;
}

int readFromFile(char filename[]) {
    FILE * file = fopen(filename, "r");
    int i = 0;
    fscanf(file, "%d", & i);
    fclose(file);
    return i;
}

void removeRedundencies(char filename[]) {
    int del = remove(filename);
    if (del)
        error_while_loading_file(filename);
}

int main(void) {
    // open files (video, text, images, vectors, errors)
    video_file = fopen("FOOTBALL_352x288_30_orig_01.yuv", "rb");
    vector_file = fopen("vector_list.txt", "w");
    mse_value_file = fopen("mse_value.txt", "w");
    prediction_img = fopen("pred_img", "wb");
    prediction_error_img = fopen("pred_error_img", "wb");
    error_text_file = fopen("error_message.txt", "a");

    // read video sequence (read binary) + close file
    fread( & frame_array, sizeof(unsigned char), size_of_2_frames, video_file);
    if (video_file == NULL) {
        error_while_loading_file("FOOTBALL_352x288_30_orig_01.yuv");
        fclose(error_text_file);
        exit(0);
    }
    fclose(video_file);

    // fill 2D buffer arrays for both frames with Y
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            buffer_frame_1[w][h] = frame_array[(h * width) + w];
            buffer_frame_2[w][h] = frame_array[((h * width) + w) + size_of_chrominances];
        }
    }

    int reading_counter = 0;

    // block matching
    for (int macro_width = 0; macro_width < width; macro_width += 16) {
        for (int macro_height = 0; macro_height < height; macro_height += 16) {
            current_block = determine_margins(macro_height, macro_width);

            // searching area
            for (int searching_area_h = current_block.h1; searching_area_h <= current_block.h0; searching_area_h++) {
                for (int searching_area_w = current_block.w1; searching_area_w <= current_block.w0; searching_area_w++) {
                    double mse = get_mse_value_per_block(macro_width, macro_height, searching_area_w, searching_area_h);
                    if (mse < mse_value) {
                        mse_value = mse;
                        current_param.h = macro_height;
                        current_param.w = macro_width;
                        current_param.h_mse = searching_area_h;
                        current_param.w_mse = searching_area_w;
                    }
                }
            }
            mse_value = 999999;
            // append line to vector-file
            fprintf(vector_file, "(%d %d) (%d %d)\n", current_param.w, current_param.h, current_param.w_mse, current_param.h_mse);

            vector_file_x = fopen("vector_list_x.txt", "w");
            vector_file_y = fopen("vector_list_y.txt", "w");
            fprintf(vector_file_x, "%d\n", current_param.w_mse);
            fprintf(vector_file_y, "%d\n", current_param.h_mse);
            fclose(vector_file_x);
            fclose(vector_file_y);
            // read from vector-file
            motion_vector_array_x[reading_counter] = readFromFile("vector_list_x.txt");
            motion_vector_array_y[reading_counter] = readFromFile("vector_list_y.txt");

            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    img_prediction[x + current_param.w][y + current_param.h] = buffer_frame_1[x + motion_vector_array_x[reading_counter]][y + motion_vector_array_y[reading_counter]];
                }
            }
            reading_counter++;
        }
    }
    fclose(vector_file);
    removeRedundencies("vector_list_x.txt"), removeRedundencies("vector_list_y.txt");
    if (prediction_img != NULL) {
        add_to_prediction_error(127);
        fwrite( & prediction_buffer, sizeof(unsigned char), width * height, prediction_img);
        fclose(prediction_img);
    } else
        error_while_loading_file("pred_img");

    // calculate mse value
    double mse = get_total_mse_value();
    fprintf(mse_value_file, "mse = %.1f", mse);
    fclose(mse_value_file);

    // printing mse to console
    // printf("mse = %.f\n", mse);

    // add 127 to the prediction error image and store it
    if (prediction_error_img != NULL) {
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++)
                error_buffer[w + h * 352] = img_prediction[w][h] - buffer_frame_2[w][h] + 127;
        }

        fwrite( & error_buffer, sizeof(unsigned char), width * height, prediction_error_img);
        fclose(prediction_error_img);
        fclose(error_text_file);
        removeRedundencies("error_message.txt");
    } else {
        error_while_loading_file("pred_error_img");
        fclose(error_text_file);
    }
    return 0;
}
