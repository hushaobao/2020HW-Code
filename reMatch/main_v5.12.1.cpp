#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define unlikely(x) __builtin_expect(!!(x), 0)

using namespace std;

#define TEST
//#define RUN_SERVER

#define Num 2000000

#define NUM_THREADS 4    // solve and save thread nums
#define THREAD_NUM_RD 32 // read data and L2 thread num
#define THREAD_NUM_SAVE 135 // 线程数最少为5 以保证每个长度的数据都可以存, 目前是讲每个长度的数据分为了6份即 5*6=30


struct Buf_star_end
{
    unsigned int star{};
    unsigned int en{};
    unsigned int cnt{};
    unsigned int cnt_m{};
    unsigned int data[200000]{};
    unsigned int data_m[100000]{};
} __attribute__((aligned((128))));

struct savedata
{
    unsigned int id{};
    unsigned int len{};
} __attribute__((aligned((128))));

struct runinfo
{
    unsigned int thread_id{};
    unsigned int start{};

    unsigned int cnt{0};
    unsigned int charcnt[THREAD_NUM_SAVE / 5][5]{};
    unsigned int ANS[20000000][7]{};
    // vector<unsigned int> idxs[5];
    vector<vector<vector<unsigned int>>> idxs;

} __attribute__((aligned((128))));


#ifdef TEST
char datafile[] = "./data/19630345/test_data.txt";
char outputfile[] = "./result/result.txt";
#endif

#ifdef RUN_SERVER
char datafile[] = "/data/test_data.txt";
char outputfile[] = "/projects/student/result.txt";
#endif


string res_length;
Buf_star_end buf_info[THREAD_NUM_RD];
runinfo runInfo[NUM_THREADS];
savedata saveData[THREAD_NUM_SAVE];

alignas(64) char *buf;

alignas(64) unsigned int test_data_size{0}, str_size{0}, Rawdata_cnt{0}, NodeCounter{0};

alignas(64) unsigned int max_id = THREAD_NUM_SAVE / 5;
alignas(64) unsigned int save_delta = 0;

alignas(64) unsigned int threadcharcounter[6]{0};

alignas(64) unsigned int subcharcnt[THREAD_NUM_SAVE / 5][5]{0};

alignas(64) unsigned int charcounter[Num];

alignas(64) unsigned int Rawdata[Num * 2];

alignas(64) unsigned int Rawdata_money[Num];

alignas(64) char path_str_d[Num][12];
alignas(64) char path_str_next[Num][12];

alignas(64) unordered_map<unsigned int, unsigned int> remapping;
alignas(64) vector<vector<unsigned int>> INVMappingdata;
alignas(64) vector<vector<unsigned int>> Mappingdata;

void *loaddata_mmapp_thr(void *Buf_info)
{
    char cur;
    auto *info = (Buf_star_end *)Buf_info;
    unsigned int s = info->star, e = info->en, ttmp{0};
    for (; s < e; ++s)
    {
        cur = *(buf + s);

        if (cur >= '0' && cur <= '9')
        {
            ttmp = ttmp * 10 + (cur - '0');
        }
        else if (unlikely(cur == ','))
        {
            info->data[info->cnt++] = ttmp;
            ttmp = 0;
        }
        else if (unlikely(cur == '\n'))
        {
            info->data_m[info->cnt_m++] = ttmp;
            ttmp = 0;
        }
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

    unsigned int last_p{0};
    unsigned int p{0}, cnt_m{0};
    for (unsigned int tc = 0; tc < THREAD_NUM_RD; ++tc)
    {
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
        for (int ii = 0; ii < cn.cnt; ++ii)
        {
            Rawdata[Rawdata_cnt++] = cn.data[ii];
        }

        for (int jj = 0; jj < cn.cnt_m; ++jj)
        {
            Rawdata_money[cnt_m++] = cn.data_m[jj];
        }
    }
}

void quickSort(vector<unsigned int>& arr, int left, int right)
{
    if (left >= right - 1)
        return;
    int i, j, base, base2, temp, temp2;
    i = left, j = right - 1;
    base = arr[left]; //取最左边的数为基准数
    base2 = arr[left + 1];
    while (i < j)
    {
        while (arr[j] >= base && i < j)
            j -= 2;
        while (arr[i] <= base && i < j)
            i += 2;
        if (i < j)
        {
            temp = arr[i];
            temp2 = arr[i + 1];

            arr[i] = arr[j];
            arr[i + 1] = arr[j + 1];

            arr[j] = temp;
            arr[j + 1] = temp2;
        }
    }
    //基准数归位
    arr[left] = arr[i];
    arr[left + 1] = arr[i + 1];
    arr[i] = base;
    arr[i + 1] = base2;
    quickSort(arr, left, i - 1);  //递归左边
    quickSort(arr, i + 2, right); //递归右边
}

void start_mapping()
{
    string trans;
    unsigned int f, s, m1, m2, cnt_m{0};

//    double start1 = clock();
    auto temp2 = Rawdata;
    unordered_set<unsigned int> u_tmp(temp2, temp2 + Rawdata_cnt);
    vector<unsigned int> temp;
    temp.assign(u_tmp.begin(), u_tmp.end());
    sort(temp.begin(), temp.end());
//    double start2 = clock();
//    remapping = unordered_map<unsigned int, unsigned int>(temp.size());
    for (unsigned int &tmp : temp)
    {
        remapping[tmp] = NodeCounter;
        trans = to_string(tmp);

        strcpy(path_str_d[NodeCounter], (trans + ",").c_str());
        strcpy(path_str_next[NodeCounter], (trans + "\n").c_str());

        charcounter[NodeCounter] = trans.size() + 1;
        ++NodeCounter;
    }
//    double start3 = clock();
    Mappingdata.reserve(NodeCounter);
    INVMappingdata.reserve(NodeCounter);

//    for (unsigned int i = 0; i < test_data_size; ++i)
//    {
//        f = remapping[Rawdata[i]], s = remapping[Rawdata[++i]];
//        Mappingdata[f].emplace_back(s);
//        Mappingdata[f].emplace_back(Rawdata_money[cnt_m]);
//        INVMappingdata[s].emplace_back(f);
//        INVMappingdata[s].emplace_back(Rawdata_money[cnt_m++]);
//    }
    for (unsigned int i = 0; i < test_data_size; i += 2)
    {
        if (unlikely(Rawdata_money[cnt_m] == 0))
        {
            ++cnt_m;
        }
        else
        {
            f = remapping[Rawdata[i]];
            s = remapping[Rawdata[i + 1]];
            m1 = Rawdata_money[cnt_m];
            m2 = Rawdata_money[cnt_m++];
            Mappingdata[f].emplace_back(s);
            Mappingdata[f].emplace_back(m1);
            INVMappingdata[s].emplace_back(f);
            INVMappingdata[s].emplace_back(m2);
        }
    }
//    double start4 = clock();
    for (unsigned int i = 0; i < NodeCounter; i++)
    {
        if (unlikely(Mappingdata[i].empty() || INVMappingdata[i].empty()))
        {
            Mappingdata[i].clear();
        }
        else
        {
            quickSort(Mappingdata[i], 0, Mappingdata[i].size() - 1);
            quickSort(INVMappingdata[i], 0, INVMappingdata[i].size() - 1);
        }
    }
//    double start5 = clock();
//    cout << (start2-start1)/CLOCKS_PER_SEC << endl;
//    cout << (start3-start2)/CLOCKS_PER_SEC << endl;
//    cout << (start4-start3)/CLOCKS_PER_SEC << endl;
//    cout << (start5-start4)/CLOCKS_PER_SEC << endl;
    save_delta = 5 * NodeCounter / THREAD_NUM_SAVE;
}

unsigned char dist[NUM_THREADS][Num];
unsigned int distmem[NUM_THREADS][Num / NUM_THREADS];
unsigned int distsize[NUM_THREADS];

void *solution_threads(void *run_info)
{
    auto *run = (runinfo *)run_info;
    unsigned int start = run->start;
    unsigned int tid = run->thread_id;
    unsigned int save_id = 0;
    unsigned int save_cnt = 0;
    unsigned int sz{0};

    unsigned int layer[6]{0};
    unsigned int layer_money[6]{0};

    unsigned char* distn = dist[tid];
    unsigned int* distmemn = distmem[tid];
    unsigned int distsizen = distsize[tid];
    queue<unsigned int> q;
    queue<unsigned char> d;

    bool is_repeat[NodeCounter]{false};
    vector<unsigned int> Invtemp;

    vector<unsigned int> L2[NodeCounter];
    vector<unsigned int> mark_isconnet_2;
    vector<bool> isconnet_2(NodeCounter, false);
    vector<bool> isconnet_3(NodeCounter, false);
    run->idxs = vector<vector<vector<unsigned int>>>(NodeCounter, vector<vector<unsigned int>>(5));

    for (unsigned int i = start; i < NodeCounter; i += NUM_THREADS)
    {
        if ((i - save_cnt) >= save_delta && save_id < max_id - 1)
        {
            ++save_id;
            save_cnt = save_id * save_delta;
        }
        if (!Mappingdata[i].empty())
        {

            for (int i1 = 0; i1 < distsizen; ++i1) {
                distn[distmemn[i1]] = 0x0f;
            }
            distsizen = 0;

            q.push(i);
            d.push(0);
            while (!q.empty()) {
                unsigned int cur_node = q.front(), cur_dist = d.front();
                q.pop(); d.pop();
                distn[cur_node] = cur_dist;
                distmemn[distsizen++] = cur_node;

                if (cur_dist < 3) {

                    const auto tmp_inv = INVMappingdata[cur_node];
                    const unsigned int nsz = tmp_inv.size();
                    for (unsigned int nid = 0; nid < nsz; ++nid) {
                        unsigned int neighbor = tmp_inv[nid];
                        if (distn[neighbor] > 3 && neighbor > i) {
                            q.push(neighbor);
                            d.push(cur_dist+1);
                        }
                    }
                }
            }

            const unsigned int s1=INVMappingdata[i].size();
            for (unsigned int k = 0; k < s1; k += 2)
            {
                if (INVMappingdata[i][k] > i)
                {
                    const unsigned int temp = INVMappingdata[i][k];
                    const unsigned int y = INVMappingdata[i][k + 1];
                    const unsigned int s2=INVMappingdata[temp].size();
                    for (unsigned int j = 0; j < s2; j += 2)
                    {
                        if (INVMappingdata[temp][j] > i)
                        {
                            const unsigned int temp2 = INVMappingdata[temp][j];
                            const unsigned int y2 = INVMappingdata[temp][j + 1];
                            if (y2 <= 5ll * y && y <= 3ll * y2)
                            {
                                L2[temp2].emplace_back(temp);
                                L2[temp2].emplace_back(y2);
                                L2[temp2].emplace_back(y);
                                isconnet_2[temp2] = true;
                                mark_isconnet_2.emplace_back(temp2);
                                const unsigned int s3=INVMappingdata[temp2].size();
                                for (unsigned int q = 0; q < s3; q += 2)
                                {
                                    if (INVMappingdata[temp2][q] > i)
                                    {
                                        const unsigned int temp3 = INVMappingdata[temp2][q];
                                        const unsigned int y3 = INVMappingdata[temp2][q + 1];
                                        if (y3 <= 5ll * y2 && y2 <= 3ll * y3)
                                        {
                                            isconnet_3[temp3] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            const unsigned int l2_size = Mappingdata[i].size();
            for (unsigned int l2 = 0; l2 < l2_size; l2 += 2)
            {
                is_repeat[i] = true;
                layer[0] = Mappingdata[i][l2];
                layer_money[0] = Mappingdata[i][l2 + 1];
                const unsigned int m0 = layer_money[0];
                if (layer[0] < i || is_repeat[layer[0]])
                    continue;
                is_repeat[layer[0]] = true;
                if (isconnet_2[layer[0]])
                {
                    Invtemp = L2[layer[0]];
                    sz = Invtemp.size();
                    for (unsigned int idx = 0; idx < sz; idx += 3)
                    {
                        if (is_repeat[Invtemp[idx]])
                            continue;
                        const unsigned int y = Invtemp[idx + 1];
                        if (m0 <= 5ll * y && y <= 3ll * m0)
                        {
                            const unsigned int y1=Invtemp[idx + 2];
                            if (y1 <= 5ll * m0 && m0 <= 3ll * y1)
                            {
                                run->ANS[run->cnt][0] = i;
                                run->ANS[run->cnt][1] = layer[0];
                                run->ANS[run->cnt][2] = Invtemp[idx];
                                run->charcnt[save_id][0] =
                                        run->charcnt[save_id][0] + charcounter[i] + charcounter[layer[0]] +
                                        charcounter[Invtemp[idx]];
                                run->idxs[i][0].emplace_back(run->cnt);
                                ++run->cnt;
                            }
                        }
                    }
                }

                const unsigned int l3_size= Mappingdata[layer[0]].size();
                for (unsigned int l3 = 0; l3 < l3_size; l3 += 2)
                {
                    layer[1] = Mappingdata[layer[0]][l3];
                    if (layer[1] < i || is_repeat[layer[1]])
                        continue;

                    layer_money[1] = Mappingdata[layer[0]][l3 + 1];
                    const unsigned int m1 = layer_money[1];
                    //div = (double)layer_money[1] / layer_money[0];
                    if (m0 <= 5ll * m1 && m1 <= 3ll * m0)
                        //if (div >= 0.2 && div <= 3)
                    {
                        is_repeat[layer[1]] = true;
                        if (isconnet_2[layer[1]])
                        {
                            Invtemp = L2[layer[1]];
                            sz = Invtemp.size();
                            for (unsigned int idx = 0; idx < sz; idx += 3)
                            {
                                // tempP = Invtemp[idx];
                                if (is_repeat[Invtemp[idx]])
                                    continue;
                                const unsigned int ttm = Invtemp[idx + 1];
                                if (m1 <= 5ll * ttm && ttm <= 3ll * m1)
                                    //if (div >= 0.2 && div <= 3)
                                {
                                    const unsigned int ttm1 = Invtemp[idx + 2];
                                    if (ttm1 <= 5ll * m0 && m0 <= 3ll * ttm1)
                                    {
                                        run->ANS[run->cnt][0] = i;
                                        run->ANS[run->cnt][1] = layer[0];
                                        run->ANS[run->cnt][2] = layer[1];
                                        run->ANS[run->cnt][3] = Invtemp[idx];
                                        run->charcnt[save_id][1] =
                                                run->charcnt[save_id][1] + charcounter[i] + charcounter[layer[0]] +
                                                charcounter[layer[1]] +
                                                charcounter[Invtemp[idx]];
                                        run->idxs[i][1].emplace_back(run->cnt);
                                        ++run->cnt;
                                    }
                                }
                            }
                        }

                        const unsigned int l4_size= Mappingdata[layer[1]].size();
                        for (unsigned int l4 = 0; l4 < l4_size; l4 += 2)
                        {
                            layer[2] = Mappingdata[layer[1]][l4];
                            if (layer[2] <= i || is_repeat[layer[2]])
                                continue;
                            layer_money[2] = Mappingdata[layer[1]][l4 + 1];
                            const unsigned int m2 = layer_money[2];
                            if (m1 <= 5ll * m2 && m2 <= 3ll * m1)
                                //div = (double)layer_money[2] / layer_money[1];
                                //if (div >= 0.2 && div <= 3)
                            {
                                is_repeat[layer[2]] = true;
                                if (isconnet_2[layer[2]])
                                {
                                    Invtemp = L2[layer[2]];
                                    sz = Invtemp.size();
                                    for (unsigned int idx = 0; idx < sz; idx += 3)
                                    {
                                        // tempP = Invtemp[idx];
                                        if (is_repeat[Invtemp[idx]])
                                            continue;
                                        const unsigned int ttm2 = Invtemp[idx + 1];
                                        if (m2 <= 5ll * ttm2 && ttm2 <= 3ll * m2)
//                                        div = (double)Invtemp[idx + 1] / layer_money[2];
//                                        if (div >= 0.2 && div <= 3)
                                        {
                                            const unsigned int ttm3 = Invtemp[idx + 2];
                                            if (ttm3 <= 5ll * m0 && m0 <= 3ll * ttm3)
//                                            div = (double)layer_money[0] / Invtemp[idx + 2];
//                                            if (div >= 0.2 && div <= 3)
                                            {
                                                run->ANS[run->cnt][0] = i;
                                                run->ANS[run->cnt][1] = layer[0];
                                                run->ANS[run->cnt][2] = layer[1];
                                                run->ANS[run->cnt][3] = layer[2];
                                                run->ANS[run->cnt][4] = Invtemp[idx];
                                                run->charcnt[save_id][2] =
                                                        run->charcnt[save_id][2] + charcounter[i] + charcounter[layer[0]] +
                                                        charcounter[layer[1]] + charcounter[layer[2]] +
                                                        charcounter[Invtemp[idx]];
                                                run->idxs[i][2].emplace_back(run->cnt);
                                                ++run->cnt;
                                            }
                                        }
                                    }
                                }

                                const unsigned int l5_size= Mappingdata[layer[2]].size();
                                for (unsigned int l5 = 0; l5 < l5_size; l5 += 2)
                                {
                                    layer[3] = Mappingdata[layer[2]][l5];
                                    if (layer[3] <= i || is_repeat[layer[3]])
                                        continue;
                                    layer_money[3] = Mappingdata[layer[2]][l5 + 1];
                                    const unsigned int m3 = layer_money[3];
                                    if (m2 <= 5ll * m3 && m3 <= 3ll * m2)
//                                    div = (double)layer_money[3] / layer_money[2];
//                                    if (div >= 0.2 && div <= 3)
                                    {
                                        is_repeat[layer[3]] = true;
                                        if (isconnet_2[layer[3]])
                                        {
                                            Invtemp = L2[layer[3]];
                                            sz = Invtemp.size();
                                            for (unsigned int idx = 0; idx < sz; idx += 3)
                                            {
                                                // tempP = Invtemp[idx];
                                                if (is_repeat[Invtemp[idx]])
                                                    continue;
                                                const unsigned int ttm4 = Invtemp[idx + 1];
                                                if (m3 <= 5ll * ttm4 && ttm4 <= 3ll * m3)
//                                                div = (double)Invtemp[idx + 1] / layer_money[3];
//                                                if (div >= 0.2 && div <= 3)
                                                {
                                                    const unsigned int ttm5 = Invtemp[idx + 2];
                                                    if (ttm5 <= 5ll * m0 && m0 <= 3ll * ttm5)
//                                                    div = (double)layer_money[0] / Invtemp[idx + 2];
//                                                    if (div >= 0.2 && div <= 3)
                                                    {
                                                        run->ANS[run->cnt][0] = i;
                                                        run->ANS[run->cnt][1] = layer[0];
                                                        run->ANS[run->cnt][2] = layer[1];
                                                        run->ANS[run->cnt][3] = layer[2];
                                                        run->ANS[run->cnt][4] = layer[3];
                                                        run->ANS[run->cnt][5] = Invtemp[idx];
                                                        run->charcnt[save_id][3] =
                                                                run->charcnt[save_id][3] + charcounter[i] + charcounter[layer[0]] +
                                                                charcounter[layer[1]] + charcounter[layer[2]] +
                                                                charcounter[layer[3]] +
                                                                charcounter[Invtemp[idx]];
                                                        run->idxs[i][3].emplace_back(run->cnt);
                                                        ++run->cnt;
                                                    }
                                                }
                                            }
                                        }

                                        if (isconnet_3[layer[3]])
                                        {
                                            const unsigned int l6_size= Mappingdata[layer[3]].size();
                                            for (unsigned int l6 = 0; l6 < l6_size; l6 += 2)
                                            {
                                                layer[4] = Mappingdata[layer[3]][l6];
                                                if (layer[4] <= i || is_repeat[layer[4]])
                                                    continue;
                                                layer_money[4] = Mappingdata[layer[3]][l6 + 1];
                                                const unsigned int m4 = layer_money[4];
                                                if (m3 <= 5ll * m4 && m4 <= 3ll * m3)
//                                                div = (double)layer_money[4] / layer_money[3];
//                                                if (div >= 0.2 && div <= 3)
                                                {
                                                    is_repeat[layer[4]] = true;
                                                    if (isconnet_2[layer[4]])
                                                    {
                                                        Invtemp = L2[layer[4]];
                                                        sz = Invtemp.size();
                                                        for (unsigned int idx = 0; idx < sz; idx += 3)
                                                        {
                                                            // tempP = Invtemp[idx];
                                                            if (is_repeat[Invtemp[idx]])
                                                                continue;
                                                            const unsigned int ttm6 = Invtemp[idx + 1];
                                                            if (m4 <= 5ll * ttm6 && ttm6 <= 3ll * m4)
//                                                            div = (double)Invtemp[idx + 1] / layer_money[4];
//                                                            if (div >= 0.2 && div <= 3)
                                                            {
                                                                const unsigned int ttm7 = Invtemp[idx + 2];
                                                                if (ttm7 <= 5ll * m0 && m0 <= 3ll * ttm7)
//                                                                div = (double)layer_money[0] / Invtemp[idx + 2];
//                                                                if (div >= 0.2 && div <= 3)
                                                                {
                                                                    run->ANS[run->cnt][0] = i;
                                                                    run->ANS[run->cnt][1] = layer[0];
                                                                    run->ANS[run->cnt][2] = layer[1];
                                                                    run->ANS[run->cnt][3] = layer[2];
                                                                    run->ANS[run->cnt][4] = layer[3];
                                                                    run->ANS[run->cnt][5] = layer[4];
                                                                    run->ANS[run->cnt][6] = Invtemp[idx];
                                                                    run->charcnt[save_id][4] =
                                                                            run->charcnt[save_id][4] + charcounter[i] +
                                                                            charcounter[layer[0]] + charcounter[layer[1]] +
                                                                            charcounter[layer[2]] + charcounter[layer[3]] +
                                                                            charcounter[layer[4]] + charcounter[Invtemp[idx]];
                                                                    run->idxs[i][4].emplace_back(run->cnt);
                                                                    ++run->cnt;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                is_repeat[layer[4]] = false;
                                            }
                                        }
                                    }
                                    is_repeat[layer[3]] = false;
                                }
                            }
                            is_repeat[layer[2]] = false;
                        }
                    }
                    is_repeat[layer[1]] = false;
                }
                is_repeat[layer[0]] = false;
            }
            is_repeat[i] = false;
        }

        for (auto &ii : mark_isconnet_2)
            L2[ii].clear();
        mark_isconnet_2.clear();
        isconnet_2 = vector<bool>(NodeCounter, false);
        isconnet_3 = vector<bool>(NodeCounter, false);
#ifdef TEST
        // if (i % 101 == 0)
        // {
        //     printf("thread:%d %d/%d\n", run->thread_id, i, NodeCounter);
        // }
#endif
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

    for (unsigned int i = 0; i < NUM_THREADS; ++i)
    {
        runInfo[i].thread_id = i;
        runInfo[i].start = i;

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
    unsigned int id = save->id;
    unsigned int len = save->len;
    int fd = open(outputfile, O_RDWR | O_CREAT, 00777);
    switch (len)
    {
        case 0: //长度为3
        {
            unsigned int offset = threadcharcounter[5];
            if (id != 0)
            {
                for (unsigned int i = 0; i < id; ++i)
                {
                    offset += subcharcnt[i][len];
                }
            }
            char *buffer = (char *)mmap(nullptr, offset + subcharcnt[id][len], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            unsigned int start = id * save_delta, end = (id + 1) * save_delta;
            if (id == max_id - 1)
                end = NodeCounter;
            // cout << start << " " << end << " " << save_delta << " " << NodeCounter << endl;
            for (unsigned int k = start; k < end; ++k)
            {
                for (auto & j : runInfo)
                {
                    for (auto &tmp : j.idxs[k][0])
                    {
                        for (unsigned int i = 0; i < 2; ++i)
                        {
                            memcpy(buffer + offset, path_str_d[j.ANS[tmp][i]], charcounter[j.ANS[tmp][i]]);
                            offset = offset + charcounter[j.ANS[tmp][i]];
                        }
                        memcpy(buffer + offset, path_str_next[j.ANS[tmp][2]], charcounter[j.ANS[tmp][2]]);
                        offset = offset + charcounter[j.ANS[tmp][2]];
                    }
                }
            }
            close(fd);
            pthread_exit(nullptr);
        }
            break;
        case 1:
        {
            unsigned int offset = threadcharcounter[5] + threadcharcounter[0];
            if (id != 0)
            {
                for (unsigned int i = 0; i < id; ++i)
                {
                    offset += subcharcnt[i][len];
                }
            }
            char *buffer = (char *)mmap(nullptr, offset + subcharcnt[id][len], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            unsigned int start = id * save_delta, end = (id + 1) * save_delta;
            if (id == max_id - 1)
                end = NodeCounter;
            for (unsigned int k = start; k < end; ++k)
            {
                for (auto & j : runInfo)
                {
                    for (auto &tmp : j.idxs[k][1])
                    {
                        for (unsigned int i = 0; i < 3; ++i)
                        {
                            memcpy(buffer + offset, path_str_d[j.ANS[tmp][i]], charcounter[j.ANS[tmp][i]]);
                            offset = offset + charcounter[j.ANS[tmp][i]];
                        }
                        memcpy(buffer + offset, path_str_next[j.ANS[tmp][3]], charcounter[j.ANS[tmp][3]]);
                        offset = offset + charcounter[j.ANS[tmp][3]];
                    }
                }
            }
            close(fd);
            pthread_exit(nullptr);
        }
            break;
        case 2:
        {
            unsigned int offset = threadcharcounter[5] + threadcharcounter[0] + threadcharcounter[1];
            if (id != 0)
            {
                for (unsigned int i = 0; i < id; ++i)
                {
                    offset += subcharcnt[i][len];
                }
            }
            char *buffer = (char *)mmap(nullptr, offset + subcharcnt[id][len], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            unsigned int start = id * save_delta, end = (id + 1) * save_delta;
            if (id == max_id - 1)
                end = NodeCounter;
            for (unsigned int k = start; k < end; ++k)
            {
                for (auto & j : runInfo)
                {
                    for (auto &tmp : j.idxs[k][2])
                    {
                        for (unsigned int i = 0; i < 4; ++i)
                        {
                            memcpy(buffer + offset, path_str_d[j.ANS[tmp][i]], charcounter[j.ANS[tmp][i]]);
                            offset = offset + charcounter[j.ANS[tmp][i]];
                        }
                        memcpy(buffer + offset, path_str_next[j.ANS[tmp][4]], charcounter[j.ANS[tmp][4]]);
                        offset = offset + charcounter[j.ANS[tmp][4]];
                    }
                }
            }
            close(fd);
            pthread_exit(nullptr);
        }
            break;
        case 3:
        {
            unsigned int offset = threadcharcounter[5] + threadcharcounter[0] + threadcharcounter[1] + threadcharcounter[2];
            if (id != 0)
            {
                for (unsigned int i = 0; i < id; ++i)
                {
                    offset += subcharcnt[i][len];
                }
            }
            char *buffer = (char *)mmap(nullptr, offset + subcharcnt[id][len], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            unsigned int start = id * save_delta, end = (id + 1) * save_delta;
            if (id == max_id - 1)
                end = NodeCounter;
            for (unsigned int k = start; k < end; ++k)
            {
                for (auto & j : runInfo)
                {
                    for (auto &tmp : j.idxs[k][3])
                    {
                        for (unsigned int i = 0; i < 5; ++i)
                        {
                            memcpy(buffer + offset, path_str_d[j.ANS[tmp][i]], charcounter[j.ANS[tmp][i]]);
                            offset = offset + charcounter[j.ANS[tmp][i]];
                        }
                        memcpy(buffer + offset, path_str_next[j.ANS[tmp][5]], charcounter[j.ANS[tmp][5]]);
                        offset = offset + charcounter[j.ANS[tmp][5]];
                    }
                }
            }
            close(fd);
            pthread_exit(nullptr);
        }
            break;
        case 4:
        {
            unsigned int offset = threadcharcounter[5] + threadcharcounter[0] + threadcharcounter[1] + threadcharcounter[2] + threadcharcounter[3];
            if (id != 0)
            {
                for (unsigned int i = 0; i < id; ++i)
                {
                    offset += subcharcnt[i][len];
                }
            }
            char *buffer = (char *)mmap(nullptr, offset + subcharcnt[id][len], PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            unsigned int start = id * save_delta, end = (id + 1) * save_delta;
            if (id == max_id - 1)
                end = NodeCounter;
            for (unsigned int k = start; k < end; ++k)
            {
                for (auto & j : runInfo)
                {
                    for (auto &tmp : j.idxs[k][4])
                    {
                        for (unsigned int i = 0; i < 6; ++i)
                        {
                            memcpy(buffer + offset, path_str_d[j.ANS[tmp][i]], charcounter[j.ANS[tmp][i]]);
                            offset = offset + charcounter[j.ANS[tmp][i]];
                        }
                        memcpy(buffer + offset, path_str_next[j.ANS[tmp][6]], charcounter[j.ANS[tmp][6]]);
                        offset = offset + charcounter[j.ANS[tmp][6]];
                    }
                }
            }
            close(fd);
            pthread_exit(nullptr);
        }
            break;
        default:
            break;
    }
}

void savemmap_threads()
{
    unsigned int pathsize = 0;
    unsigned int save_temp = THREAD_NUM_SAVE / 5;
    for (auto &i : runInfo)
    {
        pathsize = pathsize + i.cnt;
        for (unsigned int id = 0; id < save_temp; ++id)
        {
            for (unsigned int k = 0; k < 5; ++k)
            {
                threadcharcounter[k] += i.charcnt[id][k];
                subcharcnt[id][k] += i.charcnt[id][k];
            }
        }
    }

    string datatotal = to_string(pathsize) + "\n";
    threadcharcounter[5] = datatotal.size();
    int datasizetotal = threadcharcounter[5] + threadcharcounter[0] + threadcharcounter[1] + threadcharcounter[2] + threadcharcounter[3] + threadcharcounter[4];
    int fd = open(outputfile, O_RDWR | O_CREAT, 00777);
    lseek(fd, datasizetotal - 1, SEEK_SET);
    write(fd, " ", 1);
    char *buffer = (char *)mmap(nullptr, threadcharcounter[5], PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(buffer, datatotal.c_str(), threadcharcounter[5]);
    close(fd);

    pthread_t threads[THREAD_NUM_SAVE];
    pthread_attr_t attr;
    void *status;
    // 初始化并设置线程为可连接的（joinable）
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    unsigned int cnt{0}, len{0};

    for (unsigned int i = 0; i < THREAD_NUM_SAVE; ++i)
    {
        if (len >= 5)
        {
            ++cnt;
            len = 0;
        }
        saveData[i].id = cnt;
        saveData[i].len = len++;
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

#ifdef RUN_SERVER
int main()
{
    // sleep(5);
    loaddata_mmap();
    usleep(30);
    start_mapping();
    usleep(30);
    run_threads();
    usleep(30);
    // savemmap_threads_avg();
    savemmap_threads();
    return 0;
}
#endif

#ifdef TEST
int main()
{

#ifdef TEST
    auto start = std::chrono::system_clock::now();
    auto start_loaddata = std::chrono::system_clock::now();
#endif
    //    loaddata_mapping(datafile);
    loaddata_mmap();

    // cout << 2 << endl;
#ifdef TEST
    std::chrono::duration<double, std::milli> duration_loaddata =
            std::chrono::system_clock::now() - start_loaddata;
    auto start_mappingt = std::chrono::system_clock::now();
#endif

    start_mapping();
    // cout << 2 << endl;

#ifdef TEST
    std::chrono::duration<double, std::milli> duration_mappingt =
            std::chrono::system_clock::now() - start_mappingt;
    auto start_p2 = std::chrono::system_clock::now();
#endif

    // pre_creat_hoop_thread();

#ifdef TEST
    std::chrono::duration<double, std::milli> duration_p2 =
            std::chrono::system_clock::now() - start_p2;
    auto start_solution = std::chrono::system_clock::now();
#endif

    run_threads();

#ifdef TEST
    std::chrono::duration<double, std::milli> duration_solution =
            std::chrono::system_clock::now() - start_solution;
    auto start_save = std::chrono::system_clock::now();
#endif

    savemmap_threads();

#ifdef TEST
    std::chrono::duration<double, std::milli> duration_save =
            std::chrono::system_clock::now() - start_save;

    std::chrono::duration<double, std::milli> duration =
            std::chrono::system_clock::now() - start;
    // 输出显示
    unsigned int pathsize = 0;
    for (unsigned int i = 0; i < NUM_THREADS; ++i)
    {
        pathsize = pathsize + runInfo[i].cnt;
    }
    cout << "path size:" << pathsize << endl;
    cout << "loaddata Time Consume: " << duration_loaddata.count() / 1000 << "s"
         << endl;
    cout << "mapping Time Consume: " << duration_mappingt.count() / 1000 << "s"
         << endl;
    cout << "consuct map L2 Time Consume: " << duration_p2.count() / 1000 << "s"
         << endl;
    cout << "solution Time Consume: " << duration_solution.count() / 1000 << "s"
         << endl;
    cout << "save Time Consume: " << duration_save.count() / 1000 << "s" << endl;

    cout << "Time Consume: " << duration.count() / 1000 << "s" << endl;
#endif
    return 0;
}
#endif