This takes a file with a list of textfiles, compares the first of the files specified with all the other ones and returns the parallel passages (i.e. passages that are identical or nearly identical) they share.

Build the source in `src` with `make copy`.

Run the program, specifying the path of the list of text files and the minimum number of characters a parallel phrase should have, e.g. `src/copy res/tianyanlun/doclist.txt 6`.

Suppose "doclist.txt" looks like this

```/home/gesa/repositories/textanalysis/res/tianyanlun/yf_jiaoyuyuguojiazhiguanxi.txt
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_jiuwangjuelun.txt
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_lunshibianzhiji.txt
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanjinhualun.txt
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanlun_daoyan15_zuizhi.txt
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanlun_daoyan16jinwei.txt
[...]```

Then the program compares "yf_jiaoyuyuguojiazhiguanxi.txt" (first line in "doclist.txt") to all following files. I writes a file "parallels.txt" in the working directory that specifies the parallel passages found, with one parallel passage per line. Suppose *doc0* is the file compared to all other files *docX*. The output format is

**[docX filepath] [docX start] [docX end] [docX phrase] [doc0 start] [doc0 end] [doc0 phrase]**,

where **[docX start]** and **[docX end]** are the indices in the document (in terms of characters) where the phrase starts and ends.
For the example this results in

```/home/gesa/repositories/textanalysis/res/tianyanlun/yf_jiuwangjuelun.txt        4       16      作者：嚴復　清          8       20      作者：嚴復　清   
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_jiuwangjuelun.txt        8890    8895    無以易也。      697     703     無以易也。
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_lunshibianzhiji.txt      5       17      作者：嚴復　清          8       20      作者：嚴復　清   
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_lunshibianzhiji.txt      2067    2072    者也。  1717    1723    者也。今
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_lunshibianzhiji.txt      2483    2490    ，皆非狂易失心  1563    1569    ，非狂易失心
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanjinhualun.txt     5       17      作者：嚴復　清          8       20      作者：嚴復　清   
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanjinhualun.txt     2194    2200    其所以然之故    224     230     其所以然之故
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanjinhualun.txt     5493    5498    考五洲歷史      515     521     考五洲之歷史
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanlun_daoyan16jinwei.txt    1240    1245    於絕景而馳      1163    1168    於絕景而馳
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanlun_daoyan17_shanqun.txt  417     423     為何如牧乎？    1303    1309    為何如乎？由
/home/gesa/repositories/textanalysis/res/tianyanlun/yf_tianyanlun_daoyan17_shanqun.txt  527     534     者也；又必其人  555     561     者也；必其種
[...]```
