# SQL 处理插件设计文档

该插件可以处理结构化日志，使用selection clause筛选列，where clause筛选行，同时支持as重命名列，以及标量函数对列进行处理。目前暂时还不支持select from嵌套查询。

## 亮点

1. 目前对查询计划的算子结构设计比较合理，可以支持较为复杂的表达式互相嵌套，能够支持较为复杂的sql语法，具有很好的扩展性。
2. 考虑到了性能优化，目前将可以在初始化中完成的操作都尽可能地提前到了Init()函数中，减少了每次Process()的开销。
3. 以MySQL为标准，较为完善地实现了需求中要求的所有标量函数。
4. 函数的框架尽可能地与sql插件主体框架解耦开，使得无论是新的函数的开发还是将原有插件适配到sql插件中都较为容易，扩展性较好，可以复用已经存在的插件的功能。

## 代码结构

1. 处理流程
    1. 在Init()初始化期间进行对sql语句的解析以及使用解析结果生成查询计划。目前查询计划包括PredicateOperator和SelectOperator两个算子，分别对应where clause和selection clause。目前对查询计划的算子结构设计比较合理，可以支持较为复杂的表达式互相嵌套，能够支持较为复杂的sql语法，具有很好的扩展性。
    2. 在Process()处理期间，对每一条日志进行查询计划的执行，如果满足where clause，就对满足条件的日志进行selection clause的处理，最后将处理结果写入到日志中，不满足where clause的日志会被过滤掉。
2. 文件结构
    1. define.go 定义了Plan的结构，Attr、Const、Func、Cond、Case等表达式的结构、算子的结构, 同时还定义了标量函数的接口。
    2. processor_sql.go 定义了sql插件的结构，包括sql插件的配置、sql插件的初始化、sql插件的处理函数等。
    3. plan_generator.go 实现了sql插件的查询计划生成函数。
    4. plan_executor.go 实现了sql插件的查询计划执行函数。
    5. function.go 实现了除aes_encrypt()外的全部 23 个所需求的标量函数以及IF()函数。
    6. functions/aes_encrypt/aes_encrypt.go 实现了aes_encrypt()函数。为了检验函数的框架是否合理，将aes_encrypt()函数单独拆分出来，与sql插件主体框架解耦开，并且将已经存在的encrypt插件移植到了aes_encrypt()函数中。
    7. util.go 实现了一些辅助函数，包括字符串处理函数、正则表达式处理函数等。

## 性能优化

1. 将sql的解析放在了Init()函数中，避免每次Process()都初始化一次。
2. REGEXP、LIKE相关的功能会在可以提前编译pattern的情况下在解析sql语句时提前编译pattern并存起来，避免每次Process()都编译一次。
3. 在条件判断时进行了短路优化，如果前面的条件已经不满足，就不会再判断后面的条件。

## 函数移植与开发

1. 函数的框架尽可能地与sql插件主体框架解耦开，使得无论是新的函数的开发还是将原有插件适配到sql插件中都较为容易，扩展性较好，可以简单地复用已经存在的插件的功能。
2. 标量函数的框架
    1. 标量函数只需要在定义结构体后实现 "Init(param ...string) error"、"Process(param ...string) (string, error)"、"Name() string" 三个函数, 并且向FunctionMap中注册自身即可。
        1. Init()函数用于初始化函数，如果函数需要参数，就在Init()函数中解析参数，如果参数不合法，就返回error。一些能够提前处理的操作，比如编译正则表达式等，都可以放在Init()函数中。
        2. Process()函数用于执行函数处理逻辑。
        3. Name()函数用于返回函数的名字。
        4. Init()函数和Process()函数的参数都是可变参数，这样可以支持任意数量的参数。
    2. 如果要将目前已经存在的插件移植到sql插件的标量函数框架中，可以较为简单地实现，以aes_encrypt()函数为例：
        1. 改造Init()函数，功能大致不变，增加入参。
        2. 改造Process()函数，删掉原有的从models.PipelineGroupEvents中提取输入的语句，改为从入参中直接获得输入，处理逻辑大致不变。
        3. 增加Name()函数，删除Description()函数。
        4. 改造init()函数，不向 pipeline.Processors[] 中注册，而是改为向 sql.FunctionMap[] 中注册自身。
        5. 将functions/aes_encrypt这个包添加到plugins.yml中。
        6. 我删掉了部分读取密钥文件相关的逻辑，当然也可以保留下来。

## 特性

1. sql解析器
原本计划使用pingcap公司的开源TiDB parser作为解析器，但是发现TiDB parser与TiDB拆分得不是很干净，会引入对TiDB整个数据库的依赖。同样地，blastrain/vitess-sqlparser也存在这个问题，所以最终使用了另一个开源项目xwb1989/sqlparser。

2. 表达式类型支持
目前支持的表达式类型有：属性、常量、函数、条件表达式、case语句，其中函数和case语句都支持属性名、常量、条件表达式以及函数作为参数。可以支持较为复杂的sql语法。

3. 标量函数支持

    支持以下标量函数,且函数参数及语义与MySQL基本一致：

    ```sql
    COALESCE( arg1 <any type>, arg2 <any type> [, ....] )
    CONCAT()
    CONCAT_WS()
    LTRIM(string <STRING>)
    RTRIM(string <STRING>)
    TRIM(string <STRING>)
    LOWER
    UPPER
    SUBSTRING()
    SUBSTRING_INDEX()
    LENGTH()
    LOCATE()
    LEFT()
    RIGHT()
    REGEXP_INSTR()
    REGEXP_LIKE()
    REGEXP_REPLACE()
    REGEXP_SUBSTR()
    REPLACE()
    MD5()
    SHA1()
    SHA2()
    TO_BASE64() 
    AES_ENCRYPT()(该函数目前是由已有插件改造适配，与MySQL语法略有差异)
    IF()
    ```

    其中的区别在于：REGEXP_*()系列函数目前不支持mode参数，并且默认行为是区分大小写的，而MySQL默认是不区分大小写的。

4. CASE语句支持

    ```sql
    CASE WHEN condition THEN result [WHEN ...] [ELSE result] END
    CASE expr WHEN expr1 THEN result [WHEN ...] [ELSE result] END
    ```

5. 操作符支持

    ```sql
    <
    >
    =
    !=
    REGEXP
    LIKE
    ()
    NOT
    !
    AND
    OR
    ```

## 异常处理

1. 如果sql不符合MySQL语法，会在Init()函数的解析阶段抛出异常。在日志中打印error信息。
2. 如果函数找不到，会在Init()函数的查询计划生成阶段抛出异常。在日志中打印error信息。
3. 如果在数据中找不到要查询的数据，同时如果NoKeyError = true，会在Process()函数抛出异常。在日志中打印error信息，该错误如果出现在predicate()函数，该条日志会被过滤掉。

## 对任务功能需求的补充勘误

1. 在 MySQL 中，SELECT 语句不支持 IF-THEN-ELSE 条件语句。可以用IF(a, b, c)函数来实现类似的功能。IF () 语法是：IF(condition, value_if_true, value_if_false)，其中 condition 是条件表达式，value_if_true 是当条件为真时返回的值，value_if_false 是当条件为假时返回的值。
e.g.

    ```sql
    SELECT IF(avatar LIKE '%png', 'ios', 'android') AS os FROM users; 
    ```

2. 与存储过程语法不同的是，在select中，CASE 语句最后是END,而不是END CASE。

## 疑问

1. 日志中如果有"NULL"，应该将其视为空值，还是视为"NULL"字符串字面量？

## 下一步完善方向

1. 支持select from嵌套。
2. 完善细节，比如"NULL"的处理。
