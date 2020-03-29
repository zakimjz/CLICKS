# CLICKS Categorical Subspace Clustering Algorithm

CLICKS finds subspace clusters in categorical data using a k-partite clique mining approach.

Mohammed J. Zaki, Markus Peters, Ira Assent, and Thomas Seidl. CLICKS: an effective algorithm for mining subspace clusters in categorical datasets. Data and Knowledge Engineering, 60(1):51–70, January 2007. special issue on Intelligent Data Mining. doi:10.1016/j.datak.2006.01.005.

Mohammed J. Zaki, Markus Peters, Ira Assent, and Thomas Seidl. CLICKS: an effective algorithm for mining subspace clusters in categorical datasets. In 11th ACM SIGKDD International Conference onKnowledge Discovery and Data Mining. August 2005.


# HOW TO

cd click/code and run make for kcdriver

cd convert and run make for mconvert


Here's a sample session for the

mushroom dataset

1.) mconvert CSVTOCLICK mush.data mush.confusion mush.mapping 23 0 0 >
mush.click

        -> Converts mush.data (the space-separated data) to mush.click. 
        -> mush.confusion is a file where mconvert writes - for each line in
        mush.data - an integer class label.
        -> mush.mapping is a file where mconvert saves the mapping between
        integer classes (which are used by click) and the literal classes in the
        actual data file.
        -> 23: total number of columns in mush.data
        -> 0: Index of the label column (used to derive the class for
        mush.confusion)
        -> 0: Omit column 0 in the output data file

2.) ../../code/kcdriver mush.click 0.1 0.1 click.bench SUB CONFUSION MAP
mush.mapping

        -> Runs CLICK(S) with alpha = 0.1 and minsup = 0.1, benchmark results
        are appended to click.bench, subspace clusters are enabled
        -> A confusion file is created in click_confusion.txt
        -> CLICKS uses the mapping information contained in mush.mapping (see
        step 1) to output clusters in terms of actual classes (from mush.data)
        instead of integer class labels

NOTE: just type ./kcdriver for all the options. The code allows you to
      mine clusters using alpha*expectation or you can also treat
      alpha as just a minsup value (between [0,1]).

3.) mconvert CONFUSION mush.confusion click_confusion.txt dummymap.txt

        -> Output confusion matrix in LaTeX table format. The first file
        (mush.confusion) contains the actual classes, the second one the classes
        derived by CLICKS.
        -> dummymap.txt is an empty file here. You can use is to map the integer
        class labels between the two confusion files (e.g. class 1 in
        mush.confusion corresponds to class 0 in click_confusion, ...)

More infos on the two commands can be found in the appendices of the
thesis, which is attached in the most recent version.

