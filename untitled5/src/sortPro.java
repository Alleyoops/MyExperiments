import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.net.URI;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.WritableComparable;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
import org.apache.hadoop.mapreduce.lib.partition.HashPartitioner;

public class sortPro {
    static final String INPUT_PATH = "hdfs://hadoop:9000/newin";
    static final String OUT_PATH = "hdfs://hadoop:9000/newout";

    public static void main(String[] args) throws Exception {
        final Configuration configuration = new Configuration();
        final FileSystem fileSystem = FileSystem.get(new URI(INPUT_PATH), configuration);
        if (fileSystem.exists(new Path(OUT_PATH))) {
            fileSystem.delete(new Path(OUT_PATH), true);
        }
        final Job job = new Job(configuration, sortPro.class.getSimpleName());
        //指定输入文件路径
        FileInputFormat.setInputPaths(job, INPUT_PATH);
        job.setInputFormatClass(TextInputFormat.class);

        //指定自定义的Mapper类
        job.setMapperClass(MyMapper.class);
        job.setMapOutputKeyClass(NewKey.class);//指定输出<k2,v2>的类型
        job.setMapOutputValueClass(LongWritable.class);

        //指定分区类
        job.setPartitionerClass(HashPartitioner.class);
        job.setNumReduceTasks(1);

        //指定自定义的reduce类
        job.setReducerClass(MyReducer.class);
        job.setOutputKeyClass(LongWritable.class);//指定输出<k3,v3>的类型
        job.setOutputValueClass(LongWritable.class);

        //指定输出到哪里
        FileOutputFormat.setOutputPath(job, new Path(OUT_PATH));
        job.setOutputFormatClass(TextOutputFormat.class);//设定输出文件的格式化类
        job.waitForCompletion(true);//把代码提交给JobTracker执行
    }

    static class MyMapper extends Mapper<LongWritable, Text, NewKey, LongWritable> {
        //注意上面的四个类型参数，分别代表Mapper从文本读取和写进Reducer的键值对的key和value的类型
        protected void map(LongWritable key, Text value, Context context) throws java.io.IOException, InterruptedException {
            //用空格分隔开读取的每行数据
            final String[] splited = value.toString().split(" ");
            //读取一行后，把一行的三个数作为新建的NewKey类的属性（即把三个数共同作为key，那么MapReduce就会对三个数都按照NewKey类里我们自定义好的排序规则进行排序了）
            final NewKey k = new NewKey(Long.parseLong(splited[0]), Long.parseLong(splited[1]), Long.parseLong(splited[2]));
            //新建的value，因为hadoop默认排序是根据key排序的，所以和value的值无关，这里随便选择，那就以第二列的数作为value吧
            final LongWritable v = new LongWritable(Long.parseLong(splited[1]));
            //将键值对写给Reducer，键是NewKey类，值是LongWritable类，那么Reducer就要按照这种类型组合的键值对进行接收
            context.write(k, v);
        }

        ;
    }

    static class MyReducer extends Reducer<NewKey, LongWritable, String, LongWritable> {
        //注意上面的四个类型参数，分别代表Reducer从Mapper得到和写出去的键值对的key和value的类型
        protected void reduce(NewKey newKey, java.lang.Iterable<LongWritable> v2s, Context context) throws java.io.IOException, InterruptedException {
            //写出去的键值对就是要输出的东西，由于每行要输出三个数，但是键值只能输出一个key和一个value
            //所以可以尝试把Mapper传过来的newKey类的前两个数拼接为一个字符串来当做key，把第三个数作为value
            //因为键值对输出后中间会有一个制表符的间隔，为了格式的美观，所以把key的字符串中间也加一个制表符
            String outKey = new LongWritable(newKey.first) + "\t" + new LongWritable(newKey.second);
            LongWritable outValue = new LongWritable(newKey.third);
            //把键值对写出
            context.write(outKey, outValue);
        }

        ;
    }


    //这是新建的Key类，它有三个属性，保存每行的三个数
    static class NewKey implements WritableComparable<NewKey> {
        Long first;
        Long second;
        Long third;

        public NewKey() {
        }

        public NewKey(long first, long second, long third) {
            this.first = first;
            this.second = second;
            this.third = third;
        }


        @Override
        public void readFields(DataInput in) throws IOException {
            this.first = in.readLong();
            this.second = in.readLong();
            this.third = in.readLong();
        }

        @Override
        public void write(DataOutput out) throws IOException {
            out.writeLong(first);
            out.writeLong(second);
            out.writeLong(third);
        }


        //重写这个函数，自定义该类的排序规则
        @Override
        public int compareTo(NewKey o) {
            //这个方法的返回值是两个数比较之后差值，是一个int类型，至于这个差值可以不用去管，只管作为函数的返回值就行了
            //下面三行可以看出，差值是谁减去谁，就决定了排序方式是降序还是升序
            final long minus1 = o.first - this.first;//降序
            final long minus2 = o.second - this.second;//降序
            final long minus3 = this.third - o.third;//升序
            //第一个差值为0，说明第一列的两个数相等，就去判断第二列的数，不然就降序输出，然后进行下一轮判断
            if (minus1 != 0) {
                return (int) minus1;
            }
            //第二列的两个数的差值如果不为0，就降序输出，为0则说明两个数相等，需要把第三列进行一个判断
            else if (minus2 != 0) {
                return (int) minus2;
            }
            //第三列升序输出
            return (int) minus3;
            //ps:一个方法只会有一个return语句生效
        }

        @Override
        public int hashCode() {
            return this.first.hashCode() + this.second.hashCode() + this.third.hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof NewKey)) {
                return false;
            }
            NewKey newKey = (NewKey) obj;
            return (this.first == newKey.first) && (this.second == newKey.second) && (this.third == newKey.third);
        }
    }
}
