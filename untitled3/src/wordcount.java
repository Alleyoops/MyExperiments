
import java.io.IOException;
import java.util.StringTokenizer;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.GenericOptionsParser;
public class wordcount {

    public static class myMapper
            extends Mapper<Object, Text, Text, IntWritable>{

        private final static IntWritable one =new IntWritable(1);
        private Text word =new Text();

        public void map(Object key, Text value, Context context
        ) throws IOException, InterruptedException {
            StringTokenizer itr =new StringTokenizer(value.toString());
            while (itr.hasMoreTokens()) {
                word.set(itr.nextToken());
                context.write(word, one);
            }
        }
    }

    public static class myReducer
            extends Reducer<Text,IntWritable,Text,IntWritable> {
        private IntWritable result =new IntWritable();

        public void reduce(Text key, Iterable<IntWritable> values,
                           Context context
        ) throws IOException, InterruptedException {
            int sum =0;
            for (IntWritable val : values) {
                sum += val.get();
            }
            result.set(sum);
            context.write(key, result);
        }
    }

    public static void main(String[] args) throws Exception {

        Configuration conf =new Configuration();

        String[] a= {"hdfs://hadoop:9000/URL","hdfs://hadoop:9000/output2"};

        Job job =new Job(conf, "word count");    //设置一个用户定义的job名称
        job.setJarByClass(wordcount.class);
        job.setMapperClass(myMapper.class);    //为job设置Mapper类
        job.setCombinerClass(myReducer.class);    //为job设置Combiner类
        job.setReducerClass(myReducer.class);    //为job设置Reducer类
        job.setOutputKeyClass(Text.class);        //为job的输出数据设置Key类
        job.setOutputValueClass(IntWritable.class);    //为job输出设置value类

        FileInputFormat.addInputPath(job, new Path(a[0]));    //为job设置输入路径
        FileOutputFormat.setOutputPath(job, new Path(a[1]));//为job设置输出路径

        System.exit(job.waitForCompletion(true) ?0 : 1);        //运行job
    }
}