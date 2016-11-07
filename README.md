# CURL_TEST
**A Simple Demo based on boost, libcurl and openssl, in which you can do download/post/get/https get**. Other function is easy to go.

When Downloaded,you need to go to [Project->properties] to setup you own boost, libcurl and spenssl path.
**(include lib path, header file folder path, etc...)**;

And please note:
that your lib should build under **MT/MTd** mode; 
you Dll should build under **MD/MDd** mode or just use mine(in $(ProjectDir)\res\dll\);


一个基于boost、libcurl和openssl的demo，已经实现了x断点续传下载文件，发送http get和post、https get功能，其他功能扩展起来比较容易。

当下载好工程之后，你需要在工程属性中设置好自己boost库、libcurl库和openssl库的路径，包括头文件引用的目录等等

需要注意的是：
你编译的静态库必须是MT/MTd模式下生成的；动态库则是MD/MDd模式下生成的。或者干脆用我附带的Dll也行。
