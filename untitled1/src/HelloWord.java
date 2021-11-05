import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FsUrlStreamHandlerFactory;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IOUtils;

import java.io.IOException;
import java.io.InputStream;

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

public class HelloWord {
    static final String PATH = "hdfs://hadoop:9000";

    //URI是一个相对来说更广泛的概念。URL是URI的一种，是URI命名机制的一个子集，能够说URI是抽象的，而详细要使用URL来定位资源。
    public static void main(String[] args) throws URISyntaxException, IOException {
        //init
        Configuration conf = new Configuration();
        FileSystem fileSystem = FileSystem.get(new URI(PATH),conf);
        //创建目录mydir
        //mkdir("/mydir/",fileSystem);
        //上传E盘文件testfile.txt到mkdir
        //putData("E://testfile.txt","/mydir/",fileSystem);
        //读取testfile.txt文件
        read("/mydir/testfile.txt");
        //下载URL到E盘
        //getData("/URL","E://",fileSystem);
        //浏览文件夹mydir
        //list("/mydir/",fileSystem);
        //删除文件testfile.txt
        //remove("/mydir/testfile.txt",fileSystem);
        //close
        fileSystem.close();
    }



    //mkdir
    public static void mkdir(String path,FileSystem fileSystem) throws IOException {
        //创建hdfs目录
        if(fileSystem.exists(new Path(path)))
        {
            System.out.println("目录已存在");
        }
        else
        {
            boolean result=fileSystem.mkdirs(new Path(path));
            System.out.println("目录创建成功");
        }
    }

    //putData
    public static void putData(String src,String dest,FileSystem fileSystem) throws IOException {
        //src是本地文件地址，dest是hdfs的地址
        fileSystem.copyFromLocalFile(new Path(src),new Path(dest));
        System.out.println("上传成功");
    }

    //read
    public static void read(String path) throws IOException {
        URL.setURLStreamHandlerFactory(new FsUrlStreamHandlerFactory());
        final URL url = new URL(PATH+path);
        final InputStream in = url.openStream();
        IOUtils.copyBytes(in, System.out, 1024,true);
    }

    //getData
    public static void getData(String src,String dest,FileSystem fileSystem) throws IOException {
        //src是hdfs的文件地址，dest是本地文件地址
        if(!fileSystem.exists(new Path(src))){
            System.out.println("文件不存在");
        }else {
            fileSystem.copyToLocalFile(new Path(src),new Path(dest));
            System.out.println("下载成功");
        }
    }

    //list
    public static void list(String path,FileSystem fileSystem) throws IOException {
        if (!fileSystem.exists(new Path(path))) {
            System.out.println("目录不存在！");
            return;
        }
        // 得到文件的状态
        FileStatus[] status = fileSystem.listStatus(new Path(path));
        for (FileStatus s : status) {
            System.out.println(s.getPath().getName());
        }
    }

    //remove
    public static void  remove(String path,FileSystem fileSystem) throws IOException {
        if (!fileSystem.exists(new Path(path))) {
            System.out.println("文件不存在！");
        } else {
            fileSystem.delete(new Path(path), true);
            System.out.println("删除成功！");
        }
    }
}
