


#import "FMDatabase.h" // 操作数据库
#import "FMResultSet.h" // 返回操作结果
#import "FMDatabaseAdditions.h"
#import "FMDatabaseQueue.h"     // 多线程并发执行一些操作 线程安全
#import "FMDatabasePool.h"


/*
 
 Usage
 

There are three main classes in FMDB:
        // 执行一些数据库的操作
    FMDatabase - Represents a single SQLite database. Used for executing SQL statements.
        // 描述 操作 结果
    FMResultSet - Represents the results of executing a query on an FMDatabase.
        // 执行 多线程并发操作
    FMDatabaseQueue - If you're wanting to perform queries and updates on multiple threads, you'll want to use this class. It's described in the "Thread Safety" section below.
 
 
 
Database Creation
        // 三种方法创建一个数据库
    An FMDatabase is created with a path to a SQLite database file. This path can be one of these three:
 
        // 给一个系统路径, 如果硬盘上不存在这个路径,会自动创建出来
    A file system path. The file does not have to exist on disk. If it does not exist, it is created for you.
 
        // 传入一个空的字符串, 在本地的 临时路径创建要给数据库, 当数据库关闭的时候,会自动删除.(这里的temporary 是不是值沙盒里的temporary 文件夹呢???  )
    An empty string (@""). An empty database is created at a temporary location. This database is deleted with the FMDatabase connection is closed.
 
        // 传入一个 NULL 那么会在内存(非硬盘)_上创建一个数据库,当 数据库关闭的时候会被摧毁.
    NULL. An in-memory database is created. This database will be destroyed with the FMDatabase connection is closed.
        // 更多的内存数据库信息看这里哦~~
 (For more information on temporary and in-memory databases, read the sqlite documentation on the subject: http://www.sqlite.org/inmemorydb.html)
 
 
 
// 1. 创建一个数据库
 FMDatabase *db = [FMDatabase databaseWithPath:@"/tmp/tmp.db"];
// 打开数据库,
 Opening
 // 在你操作数据库之前,必须先打开它, 当权限不够或者不允许打开的时候,会打开失败. 或者会创建一个??
 Before you can interact with the database, it must be opened. Opening fails if there are insufficient resources or permissions to open and/or create the database.
 
 // 打开数据库的代码
 if (![db open]) {
 [db release];
 return;
 }
 
 
 Executing Updates
 
 // 如果不是 查找操作,那么就是 一个 update 操作,包括  (CREATE, UPDATE, INSERT, ALTER, COMMIT, BEGIN, DETACH, DELETE, DROP, END, EXPLAIN, VACUUM, and REPLACE)
 Any sort of SQL statement(声明,陈述) which is not a SELECT statement qualifies(限定,修饰,有资格) as an update. This includes CREATE, UPDATE, INSERT, ALTER, COMMIT, BEGIN, DETACH, DELETE, DROP, END, EXPLAIN, VACUUM, and REPLACE statements (plus many more). Basically, if your SQL statement does not begin with SELECT, it is an update statement.
 
 // 执行 update 操作时,如果成功,会返回YES,失败会返回 NO. lastErrorMessage,lastErrorCode 可以查看更多的信息.
 
 Executing updates returns a single value, a BOOL. A return value of YES means the update was successfully executed, and a return value of NO means that some error was encountered. You may invoke(调用) the -lastErrorMessage and -lastErrorCode methods to retrieve more information.
 
 // 执行 查询
 Executing Queries
 
 //
 A SELECT statement is a query and is executed via one of the -executeQuery... methods.
 
 // 一个查询操作会返回一个 FMResult 的对象,如果成功的话. 如果失败了,你要用 lastErrorMessage ,lastErrorCode 去确定下为啥会失败哦
 Executing queries returns an FMResultSet object if successful, and nil upon failure. You should use the -lastErrorMessage and -lastErrorCode methods to determine why a query failed.
 
 // 为了展示你你查询的结果, 可以使用一个 while 循环,来拿到他们. 一步一步的从记录中拿到哦
 In order to iterate through the results of your query, you use a while() loop. You also need to "step" from one record to the other. With FMDB, the easiest way to do that is like this:
  // 就像这样:
 FMResultSet *s = [db executeQuery:@"SELECT * FROM myTable"];
 while ([s next]) {
 //retrieve values for each record
 }
 
 // 在取查询返回的数据之前,必须要用 [FMResultSet next]
 You must always invoke -[FMResultSet next] before attempting to access the values returned in a query, even if you're only expecting one:
 
 FMResultSet *s = [db executeQuery:@"SELECT COUNT(*) FROM myTable"];
 if ([s next]) {
 int totalCount = [s intForColumnIndex:0];
 }
 
 // FMResultSet 有很多的方法用一个合适格式 去检索数据 :
 FMResultSet has many methods to retrieve data in an appropriate(恰当的) format:
 
 intForColumn:
 longForColumn:
 longLongIntForColumn:
 boolForColumn:
 doubleForColumn:
 stringForColumn:
 dateForColumn:
 dataForColumn:
 dataNoCopyForColumn:
 UTF8StringForColumnName:
 objectForColumnName:
 
 // 类似 [resuleset intForColumn: @"columnName"] ,也有 [resultSet intForColumnIndex:1];
 这种方法去检索 返回结果
 Each of these methods also has a {type}ForColumnIndex: variant that is used to retrieve the data based on the position of the column in the results, as opposed to the column's name.
 // 不需要手动的去关闭 FMResultSet ,因为当 result 被释放,或者父数据库被关闭的时候,它会自动关闭,
 Typically, there's no need to -close an FMResultSet yourself, since that happens when either the result set is deallocated, or the parent database is closed.
 
 Closing
 
 // 当 查询或者更新操作完成后,要关闭数据库,这样,SQLite 才会丢弃掉从刚才的这些操作中拿到的所有数据.
 When you have finished executing queries and updates on the database, you should -close the FMDatabase connection so that SQLite will relinquish(交出,放弃) any resources it has acquired during the course of its operation.
 
 
 // 关闭数据库
 [db close];
 
 
 Transactions
 
 //   FMDatabase 能够用一个恰当的方法 开始 和 完成一个事务,或者执行开始\结束语句
 FMDatabase can begin and commit a transaction by invoking one of the appropriate methods or executing a begin/end transaction statement.
 
 Multiple Statements and Batch Stuff
 
        (执行语句)
 // 使用 executeStatements:withResultBlock: 执行一个并联操作. 
 You can use FMDatabase's executeStatements:withResultBlock: to do multiple statements in a string:
 
 NSString *sql = @"create table bulktest1 (id integer primary key autoincrement, x text);"
 "create table bulktest2 (id integer primary key autoincrement, y text);"
 "create table bulktest3 (id integer primary key autoincrement, z text);"
 "insert into bulktest1 (x) values ('XXX');"
 "insert into bulktest2 (y) values ('YYY');"
 "insert into bulktest3 (z) values ('ZZZ');";
 
 success = [db executeStatements:sql];
 
 sql = @"select count(*) as count from bulktest1;"
 "select count(*) as count from bulktest2;"
 "select count(*) as count from bulktest3;";
 
 success = [self.db executeStatements:sql withResultBlock:^int(NSDictionary *dictionary) {
 NSInteger count = [dictionary[@"count"] integerValue];
 XCTAssertEqual(count, 1, @"expected one record for dictionary %@", dictionary);
 return 0;
 }];
 
// 数据清理
 Data Sanitization
 
 //
 When providing a SQL statement to FMDB, you should not attempt to "sanitize"(无毒,无害) any values before insertion. Instead, you should use the standard SQLite binding syntax:
 
 INSERT INTO myTable VALUES (?, ?, ?, ?)
 The ? character is recognized by SQLite as a placeholder for a value to be inserted. The execution methods all accept a variable number of arguments (or a representation of those arguments, such as an NSArray, NSDictionary, or a va_list), which are properly escaped for you.
 
 And, to use that SQL with the ? placeholders from Objective-C:
 
 NSInteger identifier = 42;
 NSString *name = @"Liam O'Flaherty (\"the famous Irish author\")";
 NSDate *date = [NSDate date];
 NSString *comment = nil;
 
 BOOL success = [db executeUpdate:@"INSERT INTO authors (identifier, name, date, comment) VALUES (?, ?, ?, ?)", @(identifier), name, date, comment ?: [NSNull null]];
 if (!success) {
 NSLog(@"error = %@", [db lastErrorMessage]);
 }
 Note: Fundamental data types, like the NSInteger variable identifier, should be as a NSNumber objects, achieved by using the @ syntax, shown above. Or you can use the [NSNumber numberWithInt:identifier] syntax, too.
 
 Likewise, SQL NULL values should be inserted as [NSNull null]. For example, in the case of comment which might be nil (and is in this example), you can use the comment ?: [NSNull null] syntax, which will insert the string if comment is not nil, but will insert [NSNull null] if it is nil.
 In Swift, you would use executeUpdate(values:), which not only is a concise Swift syntax, but also throws errors for proper Swift 2 error handling:
 
 do {
 let identifier = 42
 let name = "Liam O'Flaherty (\"the famous Irish author\")"
 let date = NSDate()
 let comment: String? = nil
 
 try db.executeUpdate("INSERT INTO authors (identifier, name, date, comment) VALUES (?, ?, ?, ?)", values: [identifier, name, date, comment ?? NSNull()])
 } catch {
 print("error = \(error)")
 }
 Note: In Swift, you don't have to wrap fundamental numeric types like you do in Objective-C. But if you are going to insert an optional string, you would probably use the comment ?? NSNull() syntax (i.e., if it is nil, use NSNull, otherwise use the string).
 Alternatively, you may use named parameters syntax:
 
 INSERT INTO authors (identifier, name, date, comment) VALUES (:identifier, :name, :date, :comment)
 The parameters must start with a colon. SQLite itself supports other characters, but internally the dictionary keys are prefixed with a colon, do not include the colon in your dictionary keys.
 
 NSDictionary *arguments = @{@"identifier": @(identifier), @"name": name, @"date": date, @"comment": comment ?: [NSNull null]};
 BOOL success = [db executeUpdate:@"INSERT INTO authors (identifier, name, date, comment) VALUES (:identifier, :name, :date, :comment)" withParameterDictionary:arguments];
 if (!success) {
 NSLog(@"error = %@", [db lastErrorMessage]);
 }
 The key point is that one should not use NSString method stringWithFormat to manually insert values into the SQL statement, itself. Nor should one Swift string interpolation to insert values into the SQL. Use ? placeholders for values to be inserted into the database (or used in WHERE clauses in SELECT statements).
 
 
 // 使用 FMDatabaseQueue 和线程安全
 Using FMDatabaseQueue and Thread Safety.
 
 
 //在并发多线程中 使用一个FMDatabase 的单例对象 并不是一个好主意. 在每一线程中去创建一个 FMdatabase 对象会是OK的, 但是不要在多线程中使用一个单例对象,也不要在同时在并发线程中使用. 不然可能会报错,崩溃等.
 Using a single instance of FMDatabase from multiple threads at once is a bad idea. It has always been OK to make a FMDatabase object per thread. Just don't share a single instance across threads, and definitely(明确的,一定的) not across multiple threads at the same time. Bad things will eventually happen and you'll eventually get something to crash, or maybe get an exception, or maybe meteorites will fall out of the sky and hit your Mac Pro. This would suck.
 
 // 所以千万不要在并发多线程中使用一个FMdatabase 的单例对象哦
 So don't instantiate a single FMDatabase object and use it across multiple threads.
 
 
 // 替代方法是 使用FMDatabaseQueue.  初始化一个单一的 FMDatabaseQueue对象,然后通过该对象去执行并发线程. FMDatabaseQueue object 会调度并且异步的执行并发线程.
 Instead, use FMDatabaseQueue. Instantiate a single FMDatabaseQueue and use it across multiple threads. The FMDatabaseQueue object will synchronize and coordinate access across the multiple threads. Here's how to use it:
 
 // 1.首先呢,初始化一个 FMDatabaseQueue 对象 queue
 First, make your queue.
 
 FMDatabaseQueue *queue = [FMDatabaseQueue databaseQueueWithPath:aPath];
 
// 然后 这样用
 
 Then use it like so:
 
 
 [queue inDatabase:^(FMDatabase *db) {
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @1];
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @2];
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @3];
 
 FMResultSet *rs = [db executeQuery:@"select * from foo"];
 while ([rs next]) {
 …
 }
 }];
 An easy way to wrap things up in a transaction can be done like this:
 
 [queue inTransaction:^(FMDatabase *db, BOOL *rollback) {
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @1];
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @2];
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @3];
 
 if (whoopsSomethingWrongHappened) {
 *rollback = YES;
 return;
 }
 // etc…
 [db executeUpdate:@"INSERT INTO myTable VALUES (?)", @4];
 }];
 The Swift equivalent would be:
 
 queue.inTransaction { db, rollback in
 do {
 try db.executeUpdate("INSERT INTO myTable VALUES (?)", values: [1])
 try db.executeUpdate("INSERT INTO myTable VALUES (?)", values: [2])
 try db.executeUpdate("INSERT INTO myTable VALUES (?)", values: [3])
 
 if whoopsSomethingWrongHappened {
 rollback.memory = true
 return
 }
 
 try db.executeUpdate("INSERT INTO myTable VALUES (?)", values: [4])
 } catch {
 rollback.memory = true
 print(error)
 }
 }
 
 // FMDatabaseQueue 将会运行 在一个串行队列中运行这些blocks, 所以如果你在一个多线程环境下同时执行FMDatabaseQueue's methods,他们将会在按照收到的消息去执行.这样的方式执行查询和更新语句将不会相互阻碍,大家都会很 happy.
 
 FMDatabaseQueue will run the blocks on a serialized queue (hence the name of the class). So if you call FMDatabaseQueue's methods from multiple threads at the same time, they will be executed in the order they are received. This way queries and updates won't step on each other's toes, and every one is happy.
 
 
 //  注意：调用 FMDatabaseQueue 方法是一个 block 。即使你在 block 中使用了 block，它也不会在其它线程中运行

 Note: The calls to FMDatabaseQueue's methods are blocking. So even though you are passing along blocks, they will not be run on another thread.
 
 // 基于blocks 创建自己的 sqlite 方法,在main函数中看看吧
 Making custom sqlite functions, based on blocks.
 

 You can do this! For an example, look for -makeFunctionNamed: in main.m
 
 Swift
 
 You can use FMDB in Swift projects too.
 
 To do this, you must:
 
 Copy the relevant .m and .h files from the FMDB src folder into your project.
 
 You can copy all of them (which is easiest), or only the ones you need. Likely you will need FMDatabase and FMResultSet at a minimum. FMDatabaseAdditions provides some very useful convenience methods, so you will likely want that, too. If you are doing multithreaded access to a database, FMDatabaseQueue is quite useful, too. If you choose to not copy all of the files from the src directory, though, you may want to update FMDB.h to only reference the files that you included in your project.
 
 Note, if you're copying all of the files from the src folder into to your project (which is recommended), you may want to drag the individual files into your project, not the folder, itself, because if you drag the folder, you won't be prompted to add the bridging header (see next point).
 
 If prompted to create a "bridging header", you should do so. If not prompted and if you don't already have a bridging header, add one.
 
 For more information on bridging headers, see Swift and Objective-C in the Same Project.
 
 In your bridging header, add a line that says:
 
 #import "FMDB.h"
 Use the variations of executeQuery and executeUpdate with the sql and values parameters with try pattern, as shown below. These renditions of executeQuery and executeUpdate both throw errors in true Swift 2 fashion.
 
 If you do the above, you can then write Swift code that uses FMDatabase. For example:
 
 let documents = try! NSFileManager.defaultManager().URLForDirectory(.DocumentDirectory, inDomain: .UserDomainMask, appropriateForURL: nil, create: false)
 let fileURL = documents.URLByAppendingPathComponent("test.sqlite")
 
 let database = FMDatabase(path: fileURL.path)
 
 if !database.open() {
 print("Unable to open database")
 return
 }
 
 do {
 try database.executeUpdate("create table test(x text, y text, z text)", values: nil)
 try database.executeUpdate("insert into test (x, y, z) values (?, ?, ?)", values: ["a", "b", "c"])
 try database.executeUpdate("insert into test (x, y, z) values (?, ?, ?)", values: ["e", "f", "g"])
 
 let rs = try database.executeQuery("select x, y, z from test", values: nil)
 while rs.next() {
 let x = rs.stringForColumn("x")
 let y = rs.stringForColumn("y")
 let z = rs.stringForColumn("z")
 print("x = \(x); y = \(y); z = \(z)")
 }
 } catch let error as NSError {
 print("failed: \(error.localizedDescription)")
 }
 
 database.close()
 History
 
 The history and changes are availbe on its GitHub page and are summarized in the "CHANGES_AND_TODO_LIST.txt" file.
 
 Contributors
 
 The contributors to FMDB are contained in the "Contributors.txt" file.
 
 Additional projects using FMDB, which might be interesting to the discerning developer.
 
 FMDBMigrationManager, A SQLite schema migration management system for FMDB: https://github.com/layerhq/FMDBMigrationManager
 FCModel, An alternative to Core Data for people who like having direct SQL access: https://github.com/marcoarment/FCModel
 Quick notes on FMDB's coding style
 
 Spaces, not tabs. Square brackets, not dot notation. Look at what FMDB already does with curly brackets and such, and stick to that style.
 
 Reporting bugs
 
 Reduce your bug down to the smallest amount of code possible. You want to make it super easy for the developers to see and reproduce your bug. If it helps, pretend that the person who can fix your bug is active on shipping 3 major products, works on a handful of open source projects, has a newborn baby, and is generally very very busy.
 
 And we've even added a template function to main.m (FMDBReportABugFunction) in the FMDB distribution to help you out:
 
 Open up fmdb project in Xcode.
 Open up main.m and modify the FMDBReportABugFunction to reproduce your bug.
 Setup your table(s) in the code.
 Make your query or update(s).
 Add some assertions which demonstrate the bug.
 Then you can bring it up on the FMDB mailing list by showing your nice and compact FMDBReportABugFunction, or you can report the bug via the github FMDB bug reporter.
 
 Optional:
 
 Figure out where the bug is, fix it, and send a patch in or bring that up on the mailing list. Make sure all the other tests run after your modifications.
 
 Support
 
 The support channels for FMDB are the mailing list (see above), filing a bug here, or maybe on Stack Overflow. So that is to say, support is provided by the community and on a voluntary basis.
 
 FMDB development is overseen by Gus Mueller of Flying Meat. If FMDB been helpful to you, consider purchasing an app from FM or telling all your friends about it.
 
 License
 
 The license for FMDB is contained in the "License.txt" file.
 
 If you happen to come across either Gus Mueller or Rob Ryan in a bar, you might consider purchasing a drink of their choosing if FMDB has been useful to you.
 
 (The drink is for them of course, shame on you for trying to keep it.)
 
 
 
 
 
 
 
 
 
 
*/
