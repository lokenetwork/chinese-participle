# chinese-participle

The all code is write in clang.And the dictionary is store in Redis.

Redis dictionary format is as below:

set 户 '{"0":"户外","1":"户型"}'

set 背 '{"0":"背包","1":"背景"}'

as you see,The first chinese word is the key of Redis.

And the most process logic is in the "main.c".

This is a picture of my pariticiple project.

 ![image](https://github.com/lokenetwork/chinese-participle/blob/master/demo-pictures/chinese-participle.png)


#The next step in this project

1/3 make web interface to manage the dictionary,may be use Node and Angular。

2/3 goods tables in mysql import is support.input the search words,the server will return all the goods id that match the participle words.It also return the result for page number.

3/3 make wrong search words be right, and give the tips to customer.
