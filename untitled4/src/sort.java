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

public class sort {
    static final String INPUT_PATH = "hdfs://hadoop:9000/in";
    static final String OUT_PATH = "hdfs://hadoop:9000/out";

    public static void main(String[] args) throws Exception {
        final Configuration configuration = new Configuration();
        final FileSystem fileSystem = FileSystem.get(new URI(INPUT_PATH), configuration);
        if (fileSystem.exists(new Path(OUT_PATH))) {
            fileSystem.delete(new Path(OUT_PATH), true);
        }
        final Job job = new Job(configuration, sort.class.getSimpleName());
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
        protected void map(LongWritable key, Text value, Context context) throws java.io.IOException, InterruptedException {
            final String[] splited = value.toString().split(" ");
            final NewKey k = new NewKey(Long.parseLong(splited[0]), Long.parseLong(splited[1]));
            final LongWritable v = new LongWritable(Long.parseLong(splited[1]));
            context.write(k, v);
        }

        ;
    }

    static class MyReducer extends Reducer<NewKey, LongWritable, LongWritable, LongWritable> {
        protected void reduce(NewKey k, java.lang.Iterable<LongWritable> v2s, Context context) throws java.io.IOException, InterruptedException {
            context.write(new LongWritable(k.first), new LongWritable(k.second));
        }

        ;
    }


    static class NewKey implements WritableComparable<NewKey> {
        Long first;
        Long second;

        public NewKey() {
        }

        public NewKey(long first, long second) {
            this.first = first;
            this.second = second;
        }


        @Override
        public void readFields(DataInput in) throws IOException {
            this.first = in.readLong();
            this.second = in.readLong();
        }

        @Override
        public void write(DataOutput out) throws IOException {
            out.writeLong(first);
            out.writeLong(second);
        }


        @Override
        public int compareTo(NewKey o) {
            final long minus = this.first - o.first;
            if (minus != 0) {
                return (int) minus;
            }
            return (int) (this.second - o.second);
        }

        @Override
        public int hashCode() {
            return this.first.hashCode() + this.second.hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof NewKey)) {
                return false;
            }
            NewKey newKey = (NewKey) obj;
            return (this.first == newKey.first) && (this.second == newKey.second);
        }
    }

}
