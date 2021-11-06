import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FsUrlStreamHandlerFactory;
import org.apache.hadoop.io.IOUtils;

import java.io.*;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class HelloFriend {
    public static void main(String[] args) throws IOException {
//        //从键盘读取输入内容
//        Scanner scanner = new Scanner(System.in);
//        //next碰到空格或者任意空字符就算结束，而nextLine全部识别
//        String s0 = scanner.nextLine();
        String s0 = read();
        //用空格分隔字符串（注意，连续两个空格会分割出“”来占用一个数组空间）
        String[] s1 = s0.split(" ");
        //创建一个存储子字符串个数的数组
        int[] count=new int[s1.length];
        //定义一个list装填去除重复项后的字符串
        List<String> list = new ArrayList<>();
        //list不存在的且不为“”的元素都放进list
        for (String s:s1) if(!list.contains(s)&&!s.equals("")) list.add(s);
        //遍历子字符串数组
        for (int i=0;i<list.size();i++) {
            //统计相同子字符串的个数
            for (String s : s1) if(list.get(i).equals(s)) count[i]++;
            //打印统计结果
            System.out.println(list.get(i)+":\t"+count[i]);
        }
    }
    //read
    public static String read() throws IOException, IOException {
        String PATH = "hdfs://hadoop:9000/URL";
        URL.setURLStreamHandlerFactory(new FsUrlStreamHandlerFactory());
        URL url = new URL(PATH);
        //获取输入字节流
        InputStream in = url.openStream();
        //读取输入流一般是采用以下两种方式
        //①
        //将字节流转换为字符流
        InputStreamReader inReader = new InputStreamReader(in);
        //从字符输入流中读取文本，缓冲各个字符，从而提供基于字符上的操作
        BufferedReader bufReader = new BufferedReader(inReader);
        String str = bufReader.readLine();
        //②
//        // 设置缓冲区域，就像搬家一样，定义一个车子来搬家
//        byte [] data = new byte[1024];
//        // 记录实际读取的长度，就像搬家一样，不是说每个都是刚好一车就装满了的
//        int len = 0;
//        //接受while循环里读取到的字符串
//        String str = new String();
//        // read将字节流读取到定义的data中，len记录每次读取的长度，当in的数据读完之后len的值则为-1
//        while( -1 != (len = in.read(data)) ) {
//            String s = new String(data, 0, len-1 , "UTF-8");
//            str = s;
//        }
        return str;
    }
}
