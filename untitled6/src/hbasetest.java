import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.*;

public class hbasetest {
    //创建表、插入记录、查询一条记录、遍历所有的记录、删除表
    public static final String TABLE_NAME = "Student";
    public static final String FAMILY_NAME_1 = "S_Info";
    public static final String FAMILY_NAME_2 = "Math";
    public static final String FAMILY_NAME_3 = "Computer_Science";
    public static final String FAMILY_NAME_4 = "English";
    public static final String[] families = new String[]{FAMILY_NAME_1,FAMILY_NAME_2,FAMILY_NAME_3,FAMILY_NAME_4};
    public static final String ROWKEY_1 = "Zhangsan";
    public static final String ROWKEY_2 = "Mary";
    public static final String ROWKEY_3 = "Lisi";
    public static final String[] rows = new String[]{ROWKEY_1,ROWKEY_2,ROWKEY_3};
    public static final String[] columns = new String[]{"S_No","S_Sex","S_Age","C_No","C_Credit","SC_Score"};
    public static final String[][] values = new String[][]{
            {"2015001","male","23","123001","2.0","86","123002","5.0","","123003","3.0","69"},
            {"2015002","famale","22","123001","2.0","","123002","5.0","77","123003","3.0","99"},
            {"2015003","male","24","123001","2.0","98","123002","5.0","95","123003","3.0",""}
    };

    public static void main(String[] args) throws Exception {
        Configuration conf = HBaseConfiguration.create();
        conf.set("hbase.rootdir", "hdfs://hadoop:9000/hbase");
        //使用eclipse时必须添加这个，否则无法定位
        conf.set("hbase.zookeeper.quorum", "hadoop");
        //创建表、删除表使用HBaseAdmin
        final HBaseAdmin hBaseAdmin = new HBaseAdmin(conf);
        //插入记录、查询一条记录、遍历所有的记录HTable
        final HTable hTable = new HTable(conf, TABLE_NAME);

//        createTable(hBaseAdmin,families);//功能1：创建表
//        addRecord(hTable,rows,families,columns,values);//功能2：添加记录
//        scanColumn(hTable,families[0],null);//功能3：浏览S_Info所有列
//        scanColumn(hTable,families[0],columns[2]);//功能3：浏览S_info的S_Age
//        deleteRow(hTable,rows[0]);//功能4：删除第一行“Zhangsan”记录
//        hTable.close();//关闭HTable
        deleteTable(hBaseAdmin);//删除表
    }
    private static void deleteTable(final HBaseAdmin hBaseAdmin)
            throws IOException {
        hBaseAdmin.disableTable(TABLE_NAME);
        hBaseAdmin.deleteTable(TABLE_NAME);
    }
    private static void createTable(final HBaseAdmin hBaseAdmin,final String[] fields)
            throws IOException {
        if(!hBaseAdmin.tableExists(TABLE_NAME)){//该表不存在,创建表
            HTableDescriptor tableDescriptor = new HTableDescriptor(TABLE_NAME);
            for (String field : fields) {
                tableDescriptor.addFamily(new HColumnDescriptor(field));
            }
            hBaseAdmin.createTable(tableDescriptor);
        }
        else
        {//该表存在过，删除原有表，再创建新的表
            deleteTable(hBaseAdmin);
            createTable(hBaseAdmin,fields);
        }
    }
    private static void addRecord(final HTable hTable,
                                  final String[] rows,
                                  final String[] families,
                                  final String[] columns,
                                  final String[][] values)
            throws IOException {
        for (int i = 0; i < 3; i++) {
            Put put = new Put(rows[i].getBytes());
            for (int j = 0; j < 4; j++) {
                if(j==0)
                {
                    put.add(families[j].getBytes(), columns[0].getBytes(), values[i][j].getBytes());
                    put.add(families[j].getBytes(), columns[1].getBytes(), values[i][j+1].getBytes());
                    put.add(families[j].getBytes(), columns[2].getBytes(), values[i][j+2].getBytes());
                }
                else
                {
                    put.add(families[j].getBytes(), columns[3].getBytes(), values[i][j*3].getBytes());
                    put.add(families[j].getBytes(), columns[4].getBytes(), values[i][j*3+1].getBytes());
                    put.add(families[j].getBytes(), columns[5].getBytes(), values[i][j*3+2].getBytes());
                }
            }
            hTable.put(put);
        }
    }
    private static void scanColumn(final HTable hTable,String family, String column) throws IOException {
        Scan scan = new Scan();
        final ResultScanner scanner = hTable.getScanner(scan);
        if(column==null)//列出family所有列
        {
            if(family.equals(families[0]))
            {
                for (Result result : scanner) {
                    final byte[] value0 = result.getValue(family.getBytes(), "S_No".getBytes());
                    final byte[] value1 = result.getValue(family.getBytes(), "S_Sex".getBytes());
                    final byte[] value2 = result.getValue(family.getBytes(), "S_Age".getBytes());
                    System.out.println(result+"\t"+new String(value0)+"\t"+new String(value1)+"\t"+new String(value2));
                }
            }
        }
        else//列出具体某一列
        {
            for (Result result : scanner) {
                final byte[] value = result.getValue(family.getBytes(), column.getBytes());
                String str = new String(value);
                if(str.equals("")) System.out.println("null");
                else System.out.println(result+"\t"+new String(value));
            }
        }
    }
    private static void deleteRow(final HTable hTable,String row) throws IOException {
        Delete delete = new Delete(row.getBytes());
        hTable.delete(delete);
        System.out.println(row+" is deleted.");
    }
    private static void getRecord(final HTable hTable) throws IOException {
        Get get = new Get(ROWKEY_1.getBytes());
        final Result result = hTable.get(get);
        final byte[] value = result.getValue(FAMILY_NAME_1.getBytes(), "age".getBytes());
        System.out.println(result+"\t"+new String(value));
    }

}