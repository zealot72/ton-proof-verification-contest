<template>
<el-container>
  <el-aside width="50%">
      <el-form label-position="right" label-width="100px">
        <el-form-item label="Search">
            <el-input
                v-model="search"
                size="mini"
                clearable
                prefix-icon="el-icon-search"
                placeholder="Type to search by asset"/>
        </el-form-item>
    </el-form>

    <el-table
        :data="assets.filter(asset => !search || asset.symbol.toLowerCase().startsWith(search.toLowerCase()))"
        :default-sort = "{prop: 'symbol', order: 'ascending'}"
        stripe
        style="width: 100%">
        <el-table-column
            label="Name"
            prop="symbol"
            sortable>
        </el-table-column>
        <el-table-column
            label="Risk"
            prop="risk"
            sortable>
            <template slot-scope="scope">
                <el-input-number
                    v-model="scope.row.risk"
                    controls-position="right"
                    clearable
                    :min="0"
                />
            </template>
        </el-table-column>
        <el-table-column
            label="Weight"
            prop="weight"
            sortable>
            <template slot-scope="scope">
                <el-input-number
                    v-model="scope.row.weight"
                    controls-position="right"
                    clearable
                    :min="0"
                />
            </template>
        </el-table-column>
    </el-table>
    <!-- <p>{{ assets }}</p> -->
  </el-aside>
  <el-main style="background-color: rgb(238, 241, 246)">
    <el-row>
        <el-col :span="12">
            <el-form :rules="riskValidationRules" label-position="right" label-width="100px">
                <el-form-item label="Agg risk">
                    <el-input :value="aggRisk" :readonly="true"></el-input>
                </el-form-item>
                <el-form-item label="Min risk">
                    <el-input-number v-model="minRisk" :min="0"></el-input-number>
                </el-form-item>
                <el-form-item label="Max risk">
                    <el-input-number v-model="maxRisk" :min="0"></el-input-number>
                </el-form-item>
            </el-form>
        </el-col>
        <el-col :span="12">
            <client-only>
                <div>
                <VueApexCharts type="pie" :options="chartOptions" :series="chartSeries" width="380"/>
                </div>
            </client-only>
        </el-col>
    </el-row>
    <el-row style="margin-top: 100px;">
        <el-col :span="12">
            <el-button type="primary">Generate proof</el-button>
        </el-col>
        <el-col :span="12">
            <el-button type="primary">Validate proof</el-button>
        </el-col>
    </el-row>
    <!-- <p>{{ chartSeries }}</p> -->
    <!-- <p>{{ chartOptions }}</p> -->
  </el-main>
</el-container>
</template>

<script>
const ASSETS = [
    {
        symbol: "AAPL",
        risk: 123,
        weight: 1000,
    },
    {
        symbol: "ABBV",
        risk: 155,
        weight: 0,
    },
    {
        symbol: "ACN",
        risk: 161,
        weight: 0,
    },
    {
        symbol: "ADBE",
        risk: 275,
        weight: 0,
    },
    {
        symbol: "AIG",
        risk: 211,
        weight: 0,
    },
    {
        symbol: "AMGN",
        risk: 191,
        weight: 0,
    },
    {
        symbol: "AMT",
        risk: 141,
        weight: 0,
    },
    {
        symbol: "AMZN",
        risk: 101,
        weight: 2000,
    },
    {
        symbol: "AVGO",
        risk: 123,
        weight: 0,
    },
    {
        symbol: "AXP",
        risk: 122,
        weight: 0,
    },
    {
        symbol: "BA",
        risk: 180,
        weight: 0,
    },
    {
        symbol: "BAC",
        risk: 140,
        weight: 0,
    },
    {
        symbol: "BIIB",
        risk: 257,
        weight: 0,
    },
    {
        symbol: "BK",
        risk: 205,
        weight: 0,
    },
    {
        symbol: "BKNG",
        risk: 119,
        weight: 0,
    },
    {
        symbol: "BLK",
        risk: 210,
        weight: 0,
    },
    {
        symbol: "BMY",
        risk: 153,
        weight: 0,
    },
    {
        symbol: "BRKA",
        risk: 63,
        weight: 0,
    },
    {
        symbol: "BRKB",
        risk: 69,
        weight: 0,
    },
    {
        symbol: "C",
        risk: 158,
        weight: 0,
    },
    {
        symbol: "CAT",
        risk: 180,
        weight: 0,
    },
    {
        symbol: "CHTR",
        risk: 193,
        weight: 0,
    },
    {
        symbol: "CL",
        risk: 111,
        weight: 0,
    },
    {
        symbol: "CMCSA",
        risk: 169,
        weight: 0,
    },
    {
        symbol: "COF",
        risk: 221,
        weight: 0,
    },
    {
        symbol: "COP",
        risk: 271,
        weight: 0,
    },
    {
        symbol: "COST",
        risk: 153,
        weight: 0,
    },
    {
        symbol: "CRM",
        risk: 208,
        weight: 0,
    },
    {
        symbol: "CSCO",
        risk: 261,
        weight: 0,
    },
    {
        symbol: "CVS",
        risk: 147,
        weight: 0,
    },
    {
        symbol: "CVX",
        risk: 179,
        weight: 0,
    },
    {
        symbol: "DD",
        risk: 126,
        weight: 0,
    },
    {
        symbol: "DHR",
        risk: 117,
        weight: 0,
    },
    {
        symbol: "DIS",
        risk: 148,
        weight: 0,
    },
    {
        symbol: "DOW",
        risk: 131,
        weight: 0,
    },
    {
        symbol: "DUK",
        risk: 161,
        weight: 0,
    },
    {
        symbol: "EMR",
        risk: 116,
        weight: 0,
    },
    {
        symbol: "EXC",
        risk: 197,
        weight: 0,
    },
    {
        symbol: "F",
        risk: 234,
        weight: 0,
    },
    {
        symbol: "FB",
        risk: 189,
        weight: 1500,
    },
    {
        symbol: "FDX",
        risk: 168,
        weight: 0,
    },
    {
        symbol: "GD",
        risk: 136,
        weight: 0,
    },
    {
        symbol: "GE",
        risk: 158,
        weight: 0,
    },
    {
        symbol: "GILD",
        risk: 203,
        weight: 0,
    },
    {
        symbol: "GM",
        risk: 163,
        weight: 0,
    },
    {
        symbol: "GOOG",
        risk: 157,
        weight: 0,
    },
    {
        symbol: "GOOGL",
        risk: 170,
        weight: 1500,
    },
    {
        symbol: "GS",
        risk: 139,
        weight: 0,
    },
    {
        symbol: "HD",
        risk: 224,
        weight: 0,
    },
    {
        symbol: "HON",
        risk: 98,
        weight: 0,
    },
    {
        symbol: "IBM",
        risk: 154,
        weight: 2000,
    },
    {
        symbol: "INTC",
        risk: 198,
        weight: 0,
    },
    {
        symbol: "JNJ",
        risk: 104,
        weight: 0,
    },
    {
        symbol: "JPM",
        risk: 114,
        weight: 0,
    },
    {
        symbol: "KHC",
        risk: 157,
        weight: 0,
    },
    {
        symbol: "KO",
        risk: 80,
        weight: 0,
    },
    {
        symbol: "LIN",
        risk: 109,
        weight: 0,
    },
    {
        symbol: "LLY",
        risk: 141,
        weight: 0,
    },
    {
        symbol: "LMT",
        risk: 148,
        weight: 0,
    },
    {
        symbol: "LOW",
        risk: 313,
        weight: 0,
    },
    {
        symbol: "MA",
        risk: 141,
        weight: 0,
    },
    {
        symbol: "MCD",
        risk: 130,
        weight: 0,
    },
    {
        symbol: "MDLZ",
        risk: 89,
        weight: 0,
    },
    {
        symbol: "MDT",
        risk: 142,
        weight: 0,
    },
    {
        symbol: "MET",
        risk: 122,
        weight: 0,
    },
    {
        symbol: "MMM",
        risk: 103,
        weight: 0,
    },
    {
        symbol: "MO",
        risk: 142,
        weight: 0,
    },
    {
        symbol: "MRK",
        risk: 125,
        weight: 0,
    },
    {
        symbol: "MS",
        risk: 152,
        weight: 0,
    },
    {
        symbol: "MSFT",
        risk: 304,
        weight: 2000,
    },
    {
        symbol: "NEE",
        risk: 287,
        weight: 0,
    },
    {
        symbol: "NFLX",
        risk: 228,
        weight: 0,
    },
    {
        symbol: "NKE",
        risk: 157,
        weight: 0,
    },
    {
        symbol: "NVDA",
        risk: 535,
        weight: 0,
    },
    {
        symbol: "ORCL",
        risk: 125,
        weight: 0,
    },
    {
        symbol: "PEP",
        risk: 87,
        weight: 0,
    },
    {
        symbol: "PFE",
        risk: 220,
        weight: 0,
    },
    {
        symbol: "PG",
        risk: 83,
        weight: 0,
    },
    {
        symbol: "PM",
        risk: 108,
        weight: 0,
    },
    {
        symbol: "PYPL",
        risk: 163,
        weight: 0,
    },
    {
        symbol: "QCOM",
        risk: 140,
        weight: 0,
    },
    {
        symbol: "RTX",
        risk: 178,
        weight: 0,
    },
    {
        symbol: "SBUX",
        risk: 118,
        weight: 0,
    },
    {
        symbol: "SO",
        risk: 227,
        weight: 0,
    },
    {
        symbol: "SPG",
        risk: 223,
        weight: 0,
    },
    {
        symbol: "T",
        risk: 149,
        weight: 0,
    },
    {
        symbol: "TGT",
        risk: 347,
        weight: 0,
    },
    {
        symbol: "TMO",
        risk: 138,
        weight: 0,
    },
    {
        symbol: "TMUS",
        risk: 207,
        weight: 0,
    },
    {
        symbol: "TSLA",
        risk: 277,
        weight: 0,
    },
    {
        symbol: "TXN",
        risk: 155,
        weight: 0,
    },
    {
        symbol: "UNH",
        risk: 104,
        weight: 0,
    },
    {
        symbol: "UNP",
        risk: 73,
        weight: 0,
    },
    {
        symbol: "UPS",
        risk: 113,
        weight: 0,
    },
    {
        symbol: "USB",
        risk: 141,
        weight: 0,
    },
    {
        symbol: "V",
        risk: 81,
        weight: 0,
    },
    {
        symbol: "VZ",
        risk: 92,
        weight: 0,
    },
    {
        symbol: "WBA",
        risk: 254,
        weight: 0,
    },
    {
        symbol: "WFC",
        risk: 129,
        weight: 0,
    },
    {
        symbol: "WMT",
        risk: 132,
        weight: 0,
    },
    {
        symbol: "XOM",
        risk: 197,
        weight: 0,
    },
]
export default {
    components: {
        VueApexCharts: () => import('vue-apexcharts')
    },
    data: function() {
        return {
            assets: ASSETS,
            search: '',
            minRisk: 0,
            maxRisk: 1000,
            riskValidationRules: {
                // aggRisk: [
                //     { required: true, message: 'Please input Activity name', trigger: 'blur' },
                //     { min: 3, max: 5, message: 'Length should be 3 to 5', trigger: 'blur' }
                // ],
                // minRisk: [
                //     { required: true, message: 'Please select minimal risk', trigger: 'change' }
                // ],
                // maxRisk: [
                //     { required: true, message: 'Please select maximal risk', trigger: 'change' }
                // ],
            }
        }
    },
    computed: {
        aggRisk() {
            if (this.weightedAssets.length === 0) {
                return 0;
            }

            const sumReducer = (accumulator, curr) => accumulator + curr;
            const sumWeight = this.weightedAssets.map(i => i.weight).reduce(sumReducer);
            if (sumWeight == 0) {
                return 0;
            }
            return this.weightedAssets.map(i => i.risk * i.weight / sumWeight).reduce(sumReducer).toFixed(3);
        },
        weightedAssets() {
            return this.assets.filter(i => i.weight > 0);
        },
        chartSeries() {
            return this.weightedAssets.map(i => i.weight)
        },
        chartOptions() {
            return {
                chart: {
                    width: 380,
                    type: 'pie',
                },
                labels: this.weightedAssets.map(i => i.symbol),
                responsive: [{
                    breakpoint: 480,
                    options: {
                        chart: {
                            width: 200
                        },
                        legend: {
                            position: 'bottom'
                        },
                    }
                }],
            }
        }
    }
}
</script>

<style>
body {
    margin: 0;
    padding: 0;
}
.el-aside {
    padding: 10px;
    height: 100vh;
}
</style>
