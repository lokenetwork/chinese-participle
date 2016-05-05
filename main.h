//
// Created by root on 4/30/16.
//
#ifndef PULLWORDS_MAIN_H
#define PULLWORDS_MAIN_H
//The max lenght of http request
#define HTTP_REQUEST_LEN 1500
//The max lenght of participle result
#define WORD_RESULT_LEN 200
//The max lenght of search words in http request
#define MAX_SEARCH_WORDS_LEN 200
//The max thread for a http request
#define MAX_CUT_WORD_THREAD_NUMBER 30
//participle result strcut
struct TREE_CUT_WORDS_RESULT {
    char pullwords_result[MAX_SEARCH_WORDS_LEN];
    int words_weight;
};
#define printd(fmt,args) printf(__FILE__ "(%d): " fmt, __LINE__, ##args);
#endif //PULLWORDS_MAIN_H
