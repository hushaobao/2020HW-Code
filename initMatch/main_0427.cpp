#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define unlikely(x) __builtin_expect(!!(x), 0)

using namespace std;

//#define TEST
#define RUN_SERVER

#define NUM_THREADS 33   // solve and save thread nums
#define THREAD_NUM_RD 1 // read data and L2 thread num
#define THREAD_NUM_SAVE 31

struct Buf_star_end
{
    unsigned int star{};
    unsigned int en{};
    unsigned int cnt{};
    unsigned int data[560010]{};
} __attribute__((aligned((128))));

struct Pre_find_hoop_info
{
    unsigned int star{};
    unsigned int en{};
} __attribute__((aligned((128))));

struct savedata
{
    unsigned int thread_id{};
} __attribute__((aligned((128))));

struct runinfo
{
    unsigned int thread_id{};
    unsigned int start{};
    unsigned int end{};

    unsigned int cnt[5]{0};
    unsigned int charcnt[5]{0};

    unsigned int P3[500000][3]{};
    unsigned int P4[500000][4]{};
    unsigned int P5[1000000][5]{};
    unsigned int P6[1500000][6]{};
    unsigned int P7[1500000][7]{};

} __attribute__((aligned((128))));

#ifdef TEST
char datafile[] = "./data/2896262/test_data.txt";
char outputfile[] = "./result/result.txt";
#endif

#ifdef RUN_SERVER
char datafile[] = "/data/test_data.txt";
char outputfile[] = "/projects/student/result.txt";
#endif

string res_length;

runinfo runInfo[NUM_THREADS];
savedata saveData[THREAD_NUM_SAVE];

alignas(64) char *buf;

alignas(64) unsigned int NodeCounter{0}, test_data_size{0}, str_size{0};

alignas(64) unsigned int remapping[280000]; // int1: Rawdata ; int2: num from 0 to n
alignas(64) unsigned int charcounter[280000];

alignas(64) unsigned int Rawdata[560010];

alignas(64) char path_str_d[230000][7];
alignas(64) char path_str_next[230000][7];

alignas(64) unsigned int INVMappingdata[230000][50]; // form who can get me
alignas(64) unsigned int Mappingdata[230000][50];

alignas(64) unsigned int INVMappingdata_cnt[230000];
alignas(64) unsigned int Mappingdata_cnt[230000];

alignas(64) unsigned int Invl2[NUM_THREADS][230000][50];
alignas(64) unsigned int Invl2cnt[NUM_THREADS][230000];

alignas(64) unsigned int charsize[THREAD_NUM_SAVE]{0};
alignas(64) unsigned int len_seek[THREAD_NUM_SAVE + 1]{0};

alignas(64) unsigned short int mulproc_start[THREAD_NUM_SAVE + 1]{0};
alignas(64) unsigned short int mulproc_end[THREAD_NUM_SAVE]{0};
alignas(64) const unsigned int mulproc_nodenum[]{3,
                                                 4, 4,
                                                 5, 5, 5, 5,
                                                 6, 6, 6, 6, 6, 6, 6, 6,
                                                 7, 7, 7, 7, 7, 7, 7, 7,
                                                 7, 7, 7, 7, 7, 7, 7, 7};

unsigned int *coutSort(unsigned int *data, unsigned int length)
{
    //确定数列最大值
    unsigned int max = data[0];
    unsigned int min = data[0];
    unsigned int tmp{0};
    for (int i = 1; i < length; ++i)
    {
        if (data[i] > max)
            max = data[i];
        if (data[i] < min)
            min = data[i];
    }
    unsigned int d = max - min + 1;
    unsigned int coutData[d]{0};
    auto *sortedArray = new unsigned int[length];

    for (int i = 0; i < length; ++i)
        ++coutData[data[i] - min];

    // 统计数组做变形，后面的元素等于前面的元素之和

    for (int i = 1; i <= d; ++i)
        coutData[i] += coutData[i - 1];

    // 倒序遍历原始数列，从统计数组找到正确的位置，输出到结果数组

    for (int i = length - 1; i >= 0; i--)
    {
        tmp = data[i] - min;
        sortedArray[coutData[tmp] - 1] = data[i];
        --coutData[tmp];

        // 找到data[i]对应的coutData的值，值为多少，表示原来排序多少，（因为从1开始，所以再减1）
        // 然后将相应的coutData的值减1，表示下次再遇到此值时，原来的排序是多少。
    }
    return sortedArray;
}

unsigned int transfer(const char *x, const short int &cnt)
{
    switch (cnt - 1)
    {
    case 1:
        return x[0] - '0';
    case 2:
        return (x[1] - '0') + (x[0] - '0') * 10;
    case 3:
        return (x[2] - '0') + (x[1] - '0') * 10 + (x[0] - '0') * 100;
    case 4:
        return (x[3] - '0') + (x[2] - '0') * 10 + (x[1] - '0') * 100 +
               (x[0] - '0') * 1000;
    case 5:
        return (x[4] - '0') + (x[3] - '0') * 10 + (x[2] - '0') * 100 +
               (x[1] - '0') * 1000 + (x[0] - '0') * 10000;
    case 6:
        return (x[5] - '0') + (x[4] - '0') * 10 + (x[3] - '0') * 100 +
               (x[2] - '0') * 1000 + (x[1] - '0') * 10000 + (x[0] - '0') * 100000;
    default:
        break;
    }
}

void *loaddata_mmapp_thr(void *buf_info)
{

    char cur, tmp[10];
    short int cnt{0};

    auto *info = (Buf_star_end *)buf_info;
    unsigned int s = info->star, e = info->en;
    for (; s < e; ++s)
    {
        cur = *(buf + s);
        tmp[cnt] = cur;
        ++cnt;
        if (cur == ',')
        {
            tmp[cnt] = '\0';
            info->data[info->cnt++] = transfer(tmp, cnt); //atoi(tmp);
            cnt = 0;
        }
        else if (unlikely(cur == '\n'))
            cnt = 0;
    }
    pthread_exit(nullptr);
}

void loaddata_mmap()
{
    // mmap test file
    int fd = open(datafile, O_RDONLY);
    unsigned int len = lseek(fd, 0, SEEK_END);
    buf = (char *)mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t tids[THREAD_NUM_RD];

    Buf_star_end buf_info[THREAD_NUM_RD];

    unsigned int last_p{0};
    unsigned int p{0}, cnt{0};
    for (unsigned int tc = 0; tc < THREAD_NUM_RD; ++tc)
    {

        //p = (tc + 1) * len / THREAD_NUM_RD;
        p = ((tc + 1) * len) >> 5;

        while (buf[p - 1] != '\n')
            --p;
        buf_info[tc].star = last_p;
        buf_info[tc].en = p;
        last_p = p;
        pthread_create(&tids[tc], nullptr, loaddata_mmapp_thr,
                       (void *)&(buf_info[tc]));
    }
    pthread_attr_destroy(&attr);
    for (unsigned long tid : tids)
    {
        pthread_join(tid, &status);
    }

    for (auto &cn : buf_info)
    {
        test_data_size += cn.cnt;
        for (int ii = 0; ii < cn.cnt; ii++)
        {
            Rawdata[cnt] = cn.data[ii];
            ++cnt;
        }
    }
}

void start_mapping()
{

    string trans;
    unsigned int num, f, s;

    unsigned int *array = coutSort(Rawdata, test_data_size);
    num = unique(array, array + test_data_size) - array;

    for (int jj = 0; jj < num; ++jj)
    {
        remapping[array[jj]] = NodeCounter;
        trans = to_string(array[jj]);

        strcpy(path_str_d[NodeCounter], (trans + ",").c_str());
        strcpy(path_str_next[NodeCounter], (trans + "\n").c_str());

        charcounter[NodeCounter] = trans.size() + 1;
        ++NodeCounter;
    }
    for (unsigned int i = 0; i < test_data_size; ++i)
    {
        f = remapping[Rawdata[i]], s = remapping[Rawdata[++i]];
        Mappingdata[f][Mappingdata_cnt[f]++] = s;
        INVMappingdata[s][INVMappingdata_cnt[s]++] = f;
    }

    for (unsigned int i = 0; i < NodeCounter; i++)
    {
        if (unlikely(Mappingdata_cnt[i] == 0 || INVMappingdata_cnt[i] == 0))
        {
            Mappingdata_cnt[i] = 0;
            //            INVMappingdata_cnt[i] = 0;
        }
        else
        {
            sort(Mappingdata[i], Mappingdata[i] + Mappingdata_cnt[i]);
            sort(INVMappingdata[i], INVMappingdata[i] + INVMappingdata_cnt[i]);
        }
    }
}

void *solution_threads(void *run_info)
{
    auto *run = (runinfo *)run_info;
    unsigned int start = run->start;
    unsigned int end = run->end;
    unsigned int id = run->thread_id;

    unsigned int sz{0};
    unsigned int *Invtemp;
    unsigned int layer[6]{0};
    bool is_repeat[NodeCounter]{false};
    vector<unsigned int> mark_isconnet_2;

    for (unsigned int i = start; i < end; ++i)
    {
        if (Mappingdata_cnt[i] != 0)
        {
            for (unsigned int k = 0; k < INVMappingdata_cnt[i]; ++k)
            {
                if (INVMappingdata[i][k] > i)
                {
                    const unsigned int temp = INVMappingdata[i][k];
                    for (unsigned int j = 0; j < INVMappingdata_cnt[temp]; ++j)
                    {
                        if (INVMappingdata[temp][j] > i)
                        {
                            Invl2[id][INVMappingdata[temp][j]]
                                 [Invl2cnt[id][INVMappingdata[temp][j]]++] = temp;
                            mark_isconnet_2.emplace_back(INVMappingdata[temp][j]);
                        }
                    }
                }
            }

            for (unsigned int l2 = 0; l2 < Mappingdata_cnt[i]; ++l2)
            {
                is_repeat[i] = true;
                layer[0] = Mappingdata[i][l2];
                if (layer[0] < i || is_repeat[layer[0]])
                    continue;
                is_repeat[layer[0]] = true;
                sz = Invl2cnt[id][layer[0]];
                if (sz != 0)
                {
                    Invtemp = Invl2[id][layer[0]];
                    for (unsigned int idx = 0; idx < sz; ++idx)
                    {
                        // tempP = Invtemp[idx];
                        if (is_repeat[Invtemp[idx]])
                            continue;
                        run->P3[run->cnt[0]][0] = i;
                        run->P3[run->cnt[0]][1] = layer[0];
                        run->P3[run->cnt[0]][2] = Invtemp[idx];
                        run->charcnt[0] =
                            run->charcnt[0] + charcounter[i] + charcounter[layer[0]] +
                            charcounter[Invtemp[idx]];
                        ++run->cnt[0];
                    }
                }
                for (unsigned int l3 = 0; l3 < Mappingdata_cnt[layer[0]]; ++l3)
                {
                    layer[1] = Mappingdata[layer[0]][l3];

                    if (layer[1] < i || is_repeat[layer[1]])
                        continue;
                    is_repeat[layer[1]] = true;
                    sz = Invl2cnt[id][layer[1]];
                    if (sz != 0)
                    {
                        Invtemp = Invl2[id][layer[1]];
                        for (unsigned int idx = 0; idx < sz; ++idx)
                        {
                            // tempP = Invtemp[idx];
                            if (is_repeat[Invtemp[idx]])
                                continue;
                            run->P4[run->cnt[1]][0] = i;
                            run->P4[run->cnt[1]][1] = layer[0];
                            run->P4[run->cnt[1]][2] = layer[1];
                            run->P4[run->cnt[1]][3] = Invtemp[idx];
                            run->charcnt[1] =
                                run->charcnt[1] + charcounter[i] + charcounter[layer[0]] +
                                charcounter[layer[1]] +
                                charcounter[Invtemp[idx]];
                            ++run->cnt[1];
                        }
                    }
                    for (unsigned int l4 = 0; l4 < Mappingdata_cnt[layer[1]]; ++l4)
                    {
                        layer[2] = Mappingdata[layer[1]][l4];
                        if (layer[2] <= i || is_repeat[layer[2]])
                            continue;
                        is_repeat[layer[2]] = true;
                        sz = Invl2cnt[id][layer[2]];
                        if (sz != 0)
                        {
                            Invtemp = Invl2[id][layer[2]];
                            for (unsigned int idx = 0; idx < sz; ++idx)
                            {
                                // tempP = Invtemp[idx];
                                if (is_repeat[Invtemp[idx]])
                                    continue;
                                run->P5[run->cnt[2]][0] = i;
                                run->P5[run->cnt[2]][1] = layer[0];
                                run->P5[run->cnt[2]][2] = layer[1];
                                run->P5[run->cnt[2]][3] = layer[2];
                                run->P5[run->cnt[2]][4] = Invtemp[idx];
                                run->charcnt[2] =
                                    run->charcnt[2] + charcounter[i] + charcounter[layer[0]] +
                                    charcounter[layer[1]] + charcounter[layer[2]] +
                                    charcounter[Invtemp[idx]];
                                ++run->cnt[2];
                            }
                        }
                        for (unsigned int l5 = 0; l5 < Mappingdata_cnt[layer[2]]; ++l5)
                        {
                            layer[3] = Mappingdata[layer[2]][l5];
                            if (layer[3] <= i || is_repeat[layer[3]])
                                continue;
                            is_repeat[layer[3]] = true;

                            sz = Invl2cnt[id][layer[3]];
                            if (sz != 0)
                            {
                                Invtemp = Invl2[id][layer[3]];
                                for (unsigned int idx = 0; idx < sz; ++idx)
                                {
                                    // tempP = Invtemp[idx];
                                    if (is_repeat[Invtemp[idx]])
                                        continue;
                                    run->P6[run->cnt[3]][0] = i;
                                    run->P6[run->cnt[3]][1] = layer[0];
                                    run->P6[run->cnt[3]][2] = layer[1];
                                    run->P6[run->cnt[3]][3] = layer[2];
                                    run->P6[run->cnt[3]][4] = layer[3];
                                    run->P6[run->cnt[3]][5] = Invtemp[idx];
                                    run->charcnt[3] =
                                        run->charcnt[3] + charcounter[i] + charcounter[layer[0]] +
                                        charcounter[layer[1]] + charcounter[layer[2]] +
                                        charcounter[layer[3]] +
                                        charcounter[Invtemp[idx]];
                                    ++run->cnt[3];
                                }
                            }
                            for (unsigned int l6 = 0; l6 < Mappingdata_cnt[layer[3]]; ++l6)
                            {
                                layer[4] = Mappingdata[layer[3]][l6];
                                if (layer[4] <= i || is_repeat[layer[4]])
                                    continue;
                                is_repeat[layer[4]] = true;

                                sz = Invl2cnt[id][layer[4]];
                                if (sz != 0)
                                {
                                    Invtemp = Invl2[id][layer[4]];
                                    for (unsigned int idx = 0; idx < sz; ++idx)
                                    {
                                        // tempP = Invtemp[idx];
                                        if (is_repeat[Invtemp[idx]])
                                            continue;
                                        run->P7[run->cnt[4]][0] = i;
                                        run->P7[run->cnt[4]][1] = layer[0];
                                        run->P7[run->cnt[4]][2] = layer[1];
                                        run->P7[run->cnt[4]][3] = layer[2];
                                        run->P7[run->cnt[4]][4] = layer[3];
                                        run->P7[run->cnt[4]][5] = layer[4];
                                        run->P7[run->cnt[4]][6] = Invtemp[idx];
                                        run->charcnt[4] =
                                            run->charcnt[4] + charcounter[i] +
                                            charcounter[layer[0]] + charcounter[layer[1]] +
                                            charcounter[layer[2]] + charcounter[layer[3]] +
                                            charcounter[layer[4]] + charcounter[Invtemp[idx]];
                                        ++run->cnt[4];
                                    }
                                }
                                is_repeat[layer[4]] = false;
                            }
                            is_repeat[layer[3]] = false;
                        }
                        is_repeat[layer[2]] = false;
                    }
                    is_repeat[layer[1]] = false;
                }
                is_repeat[layer[0]] = false;
            }
            is_repeat[i] = false;
        }
        for (auto &ii : mark_isconnet_2)
            Invl2cnt[id][ii] = 0;
        mark_isconnet_2.clear();
    }
    pthread_exit(nullptr);
}

void run_threads()
{
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    void *status;
    // 初始化并设置线程为可连接的（joinable）
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int temp = 0.35;
    runInfo[0].start = (0 * NodeCounter) / 66;
    runInfo[0].end = (temp * NodeCounter) / 66;

    runInfo[1].start = (temp * NodeCounter) / 66;
    runInfo[1].end = (2 * temp * NodeCounter) / 66;

    runInfo[2].start = (2 * temp * NodeCounter) / 66;
    runInfo[2].end = (3 * temp * NodeCounter) / 66;

    runInfo[3].start = (3 * temp * NodeCounter) / 66;
    runInfo[3].end = (4 * temp * NodeCounter) / 66;

    runInfo[4].start = (4 * temp * NodeCounter) / 66;
    runInfo[4].end = (5 * temp * NodeCounter) / 66;

    runInfo[5].start = (5 * temp * NodeCounter) / 66;
    runInfo[5].end = (6 * temp * NodeCounter) / 66;

    runInfo[6].start = (6 * temp * NodeCounter) / 66;
    runInfo[6].end = (7 * temp * NodeCounter) / 66;

    runInfo[7].start = (7 * temp * NodeCounter) / 66;
    runInfo[7].end = (8 * temp * NodeCounter) / 66;

    runInfo[8].start = (8 * temp * NodeCounter) / 66;
    runInfo[8].end = (9 * temp * NodeCounter) / 66;

    runInfo[9].start = (9 * temp * NodeCounter) / 66;
    runInfo[9].end = (10 * temp * NodeCounter) / 66;

    runInfo[10].start = (10 * temp * NodeCounter) / 66;
    runInfo[10].end = (11 * temp * NodeCounter) / 66;

    runInfo[11].start = (11 * temp * NodeCounter) / 66;
    runInfo[11].end = (12 * temp * NodeCounter) / 66;

    runInfo[12].start = (12 * temp * NodeCounter) / 66;
    runInfo[12].end = (13 * temp * NodeCounter) / 66;

    runInfo[13].start = (13 * temp * NodeCounter) / 66;
    runInfo[13].end = (14 * temp * NodeCounter) / 66;

    runInfo[14].start = (14 * temp * NodeCounter) / 66;
    runInfo[14].end = (15 * temp * NodeCounter) / 66;

    runInfo[15].start = (15 * temp * NodeCounter) / 66;
    runInfo[15].end = (16 * temp * NodeCounter) / 66;

    runInfo[16].start = (16 * temp * NodeCounter) / 66;
    runInfo[16].end = (17 * temp * NodeCounter) / 66;

    runInfo[17].start = (17 * temp * NodeCounter) / 66;
    runInfo[17].end = (18 * temp * NodeCounter) / 66;

    runInfo[18].start = (18 * temp * NodeCounter) / 66;
    runInfo[18].end = (19 * temp * NodeCounter) / 66;

    runInfo[19].start = (19 * temp * NodeCounter) / 66;
    runInfo[19].end = (20 * temp * NodeCounter) / 66;

    runInfo[20].start = (20 * temp * NodeCounter) / 66;
    runInfo[20].end = (21 * temp * NodeCounter) / 66;

    runInfo[21].start = (21 * temp * NodeCounter) / 66;
    runInfo[21].end = (22 * temp * NodeCounter) / 66;

    runInfo[22].start = (22 * temp * NodeCounter) / 66;
    runInfo[22].end = (23 * temp * NodeCounter) / 66;

    runInfo[23].start = (23 * temp * NodeCounter) / 66;
    runInfo[23].end = (24 * temp * NodeCounter) / 66;

    runInfo[24].start = (24 * temp * NodeCounter) / 66;
    runInfo[24].end = (25 * temp * NodeCounter) / 66;

    runInfo[25].start = (25 * temp * NodeCounter) / 66;
    runInfo[25].end = (26 * temp * NodeCounter) / 66;

    runInfo[26].start = (26 * temp * NodeCounter) / 66;
    runInfo[26].end = (27 * temp * NodeCounter) / 66;

    runInfo[27].start = (27 * temp * NodeCounter) / 66;
    runInfo[27].end = (28 * temp * NodeCounter) / 66;

    runInfo[28].start = (28 * temp * NodeCounter) / 66;
    runInfo[28].end = (29 * temp * NodeCounter) / 66;

    runInfo[29].start = (29 * temp * NodeCounter) / 66;
    runInfo[29].end = (30 * temp * NodeCounter) / 66;

    runInfo[30].start = (30 * temp * NodeCounter) / 66;
    runInfo[30].end = (31 * temp * NodeCounter) / 66;

    runInfo[31].start = (31 * temp * NodeCounter) / 66;
    runInfo[31].end = (32 * temp * NodeCounter) / 66;

    runInfo[32].start = (32 * temp * NodeCounter) / 66;
    runInfo[32].end = (66 * NodeCounter) / 66;

    for (unsigned int i = 0; i < NUM_THREADS; ++i)
    {
        runInfo[i].thread_id = i;
        // runInfo[i].start = (i * NodeCounter) / NUM_THREADS;
        // runInfo[i].end = ((i + 1) * NodeCounter) / NUM_THREADS;

        // cout << "main() : creating thread, " << i << endl;
        pthread_create(&threads[i], nullptr, solution_threads,
                       (void *)&(runInfo[i]));
    }
    // 删除属性，并等待其他线程
    pthread_attr_destroy(&attr);
    for (unsigned long thread : threads)
    {
        pthread_join(thread, &status);
    }
}

void *save_threads(void *save_Data)
{
    auto *save = (savedata *)save_Data;
    unsigned int id = save->thread_id;
    if (charsize[id] == 0)
        pthread_exit(nullptr);
    unsigned short int proc_start = mulproc_start[id], proc_end = mulproc_end[id],
                       proc_node_num = mulproc_nodenum[id];

    int fd = open(outputfile, O_RDWR | O_CREAT, 00777);

    switch (proc_node_num)
    {
    case 3:
    {
        unsigned int offset = len_seek[id];
        char *buffer = (char *)mmap(nullptr, offset + charsize[id],
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (id == 0)
        {
            memcpy(buffer, res_length.c_str(), res_length.size());
            offset = res_length.size();
        }

        for (; proc_start < proc_end; ++proc_start)
        {
            for (unsigned int k = 0; k < runInfo[proc_start].cnt[0]; ++k)
            {
                const auto ttmp = runInfo[proc_start].P3[k];
                for (unsigned int i = 0; i < 2; ++i)
                {
                    memcpy(buffer + offset,
                           path_str_d[ttmp[i]],
                           charcounter[ttmp[i]]);
                    offset = offset + charcounter[ttmp[i]];
                }
                memcpy(buffer + offset,
                       path_str_next[ttmp[2]],
                       charcounter[ttmp[2]]);
                offset = offset + charcounter[ttmp[2]];
            }
        }
        close(fd);
        pthread_exit(nullptr);
        break;
    }
    case 4:
    {
        unsigned int offset = len_seek[id];
        char *buffer = (char *)mmap(nullptr, offset + charsize[id],
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        for (; proc_start < proc_end; ++proc_start)
        {
            for (unsigned int k = 0; k < runInfo[proc_start].cnt[1]; ++k)
            {
                const auto ttmp = runInfo[proc_start].P4[k];
                for (unsigned int i = 0; i < 3; ++i)
                {
                    memcpy(buffer + offset,
                           path_str_d[ttmp[i]],
                           charcounter[ttmp[i]]);
                    offset = offset + charcounter[ttmp[i]];
                }
                memcpy(buffer + offset,
                       path_str_next[ttmp[3]],
                       charcounter[ttmp[3]]);
                offset = offset + charcounter[ttmp[3]];
            }
        }
        close(fd);
        pthread_exit(nullptr);
        break;
    }
    case 5:
    {
        unsigned int offset = len_seek[id];
        char *buffer = (char *)mmap(nullptr, offset + charsize[id],
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        for (; proc_start < proc_end; ++proc_start)
        {
            for (unsigned int k = 0; k < runInfo[proc_start].cnt[2]; ++k)
            {
                const auto ttmp = runInfo[proc_start].P5[k];
                for (unsigned int i = 0; i < 4; ++i)
                {
                    memcpy(buffer + offset,
                           path_str_d[ttmp[i]],
                           charcounter[ttmp[i]]);
                    offset = offset + charcounter[ttmp[i]];
                }
                memcpy(buffer + offset,
                       path_str_next[ttmp[4]],
                       charcounter[ttmp[4]]);
                offset = offset + charcounter[ttmp[4]];
            }
        }
        close(fd);
        pthread_exit(nullptr);
        break;
    }
    case 6:
    {
        unsigned int offset = len_seek[id];
        char *buffer = (char *)mmap(NULL, offset + charsize[id],
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        for (; proc_start < proc_end; ++proc_start)
        {
            for (unsigned int k = 0; k < runInfo[proc_start].cnt[3]; ++k)
            {
                const auto ttmp = runInfo[proc_start].P6[k];
                for (unsigned int i = 0; i < 5; ++i)
                {
                    memcpy(buffer + offset,
                           path_str_d[ttmp[i]],
                           charcounter[ttmp[i]]);
                    offset = offset + charcounter[ttmp[i]];
                }
                memcpy(buffer + offset,
                       path_str_next[ttmp[5]],
                       charcounter[ttmp[5]]);
                offset = offset + charcounter[ttmp[5]];
            }
        }
        close(fd);
        pthread_exit(nullptr);
        break;
    }
    case 7:
    {
        unsigned int offset = len_seek[id];
        char *buffer = (char *)mmap(nullptr, offset + charsize[id],
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        for (; proc_start < proc_end; ++proc_start)
        {
            for (unsigned int k = 0; k < runInfo[proc_start].cnt[4]; ++k)
            {
                const auto ttmp = runInfo[proc_start].P7[k];
                for (unsigned int i = 0; i < 6; ++i)
                {
                    memcpy(buffer + offset,
                           path_str_d[ttmp[i]],
                           charcounter[ttmp[i]]);
                    offset = offset + charcounter[ttmp[i]];
                }
                memcpy(buffer + offset,
                       path_str_next[ttmp[6]],
                       charcounter[ttmp[6]]);
                offset = offset + charcounter[ttmp[6]];
            }
        }
        close(fd);
        pthread_exit(nullptr);
        break;
    }
    default:
        break;
    }
}
void savemmap_threads_avg()
{

    unsigned int tmp[32]{0}, per_hoop_num[5]{0}, tmp_hoop_num[5]{0}, hoop5{0}, hoop6{0}, hoop7{0};
    unsigned int pathsize(0), cnt(0), cntt{0}; // , ttm(0);

    for (auto &x : runInfo)
    {
        per_hoop_num[0] += x.cnt[0];
        per_hoop_num[1] += x.cnt[1];
        per_hoop_num[2] += x.cnt[2];
        per_hoop_num[3] += x.cnt[3];
        per_hoop_num[4] += x.cnt[4];
    }

    hoop5 = (per_hoop_num[2] >> 2) + 1;
    hoop6 = (per_hoop_num[3] >> 3) + 1;
    hoop7 = (per_hoop_num[4] >> 4) + 1;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        const auto &x = runInfo[i];

        charsize[0] += x.charcnt[0];
        // hoop 4
        if (tmp_hoop_num[1] < (per_hoop_num[1] >> 1))
        {
            charsize[1] += x.charcnt[1];
            mulproc_end[1] = i;
        }
        else
        {
            charsize[2] += x.charcnt[1];
        }
        // hoop 5
        switch (tmp_hoop_num[2] / hoop5)
        {
        case 0:
        {
            charsize[3] += x.charcnt[2];
            mulproc_end[3] = i;
            break;
        }
        case 1:
        {
            charsize[4] += x.charcnt[2];
            mulproc_end[4] = i;
            break;
        }
        case 2:
        {
            charsize[5] += x.charcnt[2];
            mulproc_end[5] = i;
            break;
        }
        case 3:
        {
            charsize[6] += x.charcnt[2];
            break;
        }
        }

        // hoop 6
        switch (tmp_hoop_num[3] / hoop6)
        {
        case 0:
        {
            charsize[7] += x.charcnt[3];
            mulproc_end[7] = i;
            break;
        }
        case 1:
        {
            charsize[8] += x.charcnt[3];
            mulproc_end[8] = i;
            break;
        }
        case 2:
        {
            charsize[9] += x.charcnt[3];
            mulproc_end[9] = i;
            break;
        }
        case 3:
        {
            charsize[10] += x.charcnt[3];
            mulproc_end[10] = i;
            break;
        }
        case 4:
        {
            charsize[11] += x.charcnt[3];
            mulproc_end[11] = i;
            break;
        }
        case 5:
        {
            charsize[12] += x.charcnt[3];
            mulproc_end[12] = i;
            break;
        }
        case 6:
        {
            charsize[13] += x.charcnt[3];
            mulproc_end[13] = i;
            break;
        }
        case 7:
        {
            charsize[14] += x.charcnt[3];
            //                mulproc_end[14] = i;
            break;
        }
        }

        // hoop7
        switch (tmp_hoop_num[4] / hoop7)
        {
        case 0:
        {
            charsize[15] += x.charcnt[4];
            mulproc_end[15] = i;
            break;
        }
        case 1:
        {
            charsize[16] += x.charcnt[4];
            mulproc_end[16] = i;
            break;
        }
        case 2:
        {
            charsize[17] += x.charcnt[4];
            mulproc_end[17] = i;
            break;
        }
        case 3:
        {
            charsize[18] += x.charcnt[4];
            mulproc_end[18] = i;
            break;
        }
        case 4:
        {
            charsize[19] += x.charcnt[4];
            mulproc_end[19] = i;
            break;
        }
        case 5:
        {
            charsize[20] += x.charcnt[4];
            mulproc_end[20] = i;
            break;
        }
        case 6:
        {
            charsize[21] += x.charcnt[4];
            mulproc_end[21] = i;
            break;
        }
        case 7:
        {
            charsize[22] += x.charcnt[4];
            mulproc_end[22] = i;
            break;
        }
        case 8:
        {
            charsize[23] += x.charcnt[4];
            mulproc_end[23] = i;
            break;
        }
        case 9:
        {
            charsize[24] += x.charcnt[4];
            mulproc_end[24] = i;
            break;
        }
        case 10:
        {
            charsize[25] += x.charcnt[4];
            mulproc_end[25] = i;
            break;
        }
        case 11:
        {
            charsize[26] += x.charcnt[4];
            mulproc_end[26] = i;
            break;
        }
        case 12:
        {
            charsize[27] += x.charcnt[4];
            mulproc_end[27] = i;
            break;
        }
        case 13:
        {
            charsize[28] += x.charcnt[4];
            mulproc_end[28] = i;
            break;
        }
        case 14:
        {
            charsize[29] += x.charcnt[4];
            mulproc_end[29] = i;
            break;
        }
        case 15:
        {
            charsize[30] += x.charcnt[4];
            //                mulproc_end[30] = i;
            break;
        }
        }

        //        tmp_hoop_num[0] += x.cnt[0];
        tmp_hoop_num[1] += x.cnt[1];
        tmp_hoop_num[2] += x.cnt[2];
        tmp_hoop_num[3] += x.cnt[3];
        tmp_hoop_num[4] += x.cnt[4];
    }

    pathsize = per_hoop_num[0] + per_hoop_num[1] + per_hoop_num[2] + per_hoop_num[3] + per_hoop_num[4];
    res_length = to_string(pathsize) + "\n";

    charsize[0] += res_length.size();

    mulproc_end[0] = mulproc_end[2] = mulproc_end[6] = mulproc_end[14] = mulproc_end[30] = NUM_THREADS;
    for (int ii = 0; ii < THREAD_NUM_SAVE; ii++)
    {
        mulproc_end[ii] += 1;
        str_size += charsize[ii];
        len_seek[++cnt] = len_seek[ii] + charsize[ii];

        if (charsize[ii] != 0)
            tmp[cntt++] = mulproc_end[ii];
    }
    mulproc_end[0] = mulproc_end[2] = mulproc_end[6] = mulproc_end[14] = mulproc_end[30] = NUM_THREADS;
    cntt = 0;
    for (int ii = 1; ii < THREAD_NUM_SAVE; ii++)
    {
        if (charsize[ii] != 0)
        {
            mulproc_start[ii] = tmp[cntt++];
        }
    }

    mulproc_start[0] = mulproc_start[1] = mulproc_start[3] = mulproc_start[7] = mulproc_start[15] = 0;

    int fd = open(outputfile, O_RDWR | O_CREAT, 00777);
    lseek(fd, str_size - 1, SEEK_SET);
    write(fd, "\0", 1);
    close(fd);

    pthread_t threads[THREAD_NUM_SAVE];
    pthread_attr_t attr;
    void *status;
    // 初始化并设置线程为可连接的（joinable）
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (unsigned int i = 0; i < THREAD_NUM_SAVE; ++i)
    {
        saveData[i].thread_id = i;
        pthread_create(&threads[i], nullptr, save_threads, (void *)&(saveData[i]));
    }
    // 删除属性，并等待其他线程
    pthread_attr_destroy(&attr);
    for (unsigned long thread : threads)
    {
        pthread_join(thread, &status);
    }
#ifdef RUN_SERVER
    exit(0);
#endif
}

int main()
{
    sleep(5);
    loaddata_mmap();
    usleep(50);
    start_mapping();
    usleep(50);
    run_threads();
    usleep(50);
    savemmap_threads_avg();
    return 0;
}

// int main()
// {

// #ifdef TEST
//     auto start = std::chrono::system_clock::now();
//     auto start_loaddata = std::chrono::system_clock::now();
// #endif
//        loaddata_mapping(datafile);
//     loaddata_mmap();

// #ifdef TEST
//     std::chrono::duration<double, std::milli> duration_loaddata =
//         std::chrono::system_clock::now() - start_loaddata;
//     auto start_mappingt = std::chrono::system_clock::now();
// #endif

//     start_mapping();

// #ifdef TEST
//     std::chrono::duration<double, std::milli> duration_mappingt =
//         std::chrono::system_clock::now() - start_mappingt;
//     auto start_p2 = std::chrono::system_clock::now();
// #endif

//     pre_creat_hoop_thread();

// #ifdef TEST
//     std::chrono::duration<double, std::milli> duration_p2 =
//         std::chrono::system_clock::now() - start_p2;
//     auto start_solution = std::chrono::system_clock::now();
// #endif
//     solution_recursive();
//     run_threads();

// #ifdef TEST
//     std::chrono::duration<double, std::milli> duration_solution =
//         std::chrono::system_clock::now() - start_solution;
//     auto start_save = std::chrono::system_clock::now();
// #endif

//     savemmap(outputfile);
//        savemmap_threads();
//     savemmap_threads_avg();

// #ifdef TEST
//     std::chrono::duration<double, std::milli> duration_save =
//         std::chrono::system_clock::now() - start_save;

//     std::chrono::duration<double, std::milli> duration =
//         std::chrono::system_clock::now() - start;
//     输出显示
//     unsigned int pathsize = 0;
//     for (unsigned int i = 0; i < NUM_THREADS; ++i)
//     {
//         pathsize = pathsize + runInfo[i].cnt[0] + runInfo[i].cnt[1] +
//                    runInfo[i].cnt[2] + runInfo[i].cnt[3] + runInfo[i].cnt[4];
//     }
//     cout << "path size:" << pathsize << endl;
//     cout << "loaddata Time Consume: " << duration_loaddata.count() / 1000 << "s"
//          << endl;
//     cout << "mapping Time Consume: " << duration_mappingt.count() / 1000 << "s"
//          << endl;
//     cout << "consuct map L2 Time Consume: " << duration_p2.count() / 1000 << "s"
//          << endl;
//     cout << "solution Time Consume: " << duration_solution.count() / 1000 << "s"
//          << endl;
//     cout << "save Time Consume: " << duration_save.count() / 1000 << "s" << endl;

//     cout << "Time Consume: " << duration.count() / 1000 << "s" << endl;
// #endif
//     return 0;
// }