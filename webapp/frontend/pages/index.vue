<template>
<el-container>
  <el-aside width="50%">
      <el-form label-position="right" label-width="100px">
        <el-row>
            <el-col :span="4">
                <el-form-item label="only weighted">
                    <el-checkbox v-model="filterWeighted"/>
                </el-form-item>
            </el-col>
            <el-col :span="20">
                <el-form-item label="Search">
                    <el-input
                        v-model="search"
                        size="mini"
                        clearable
                        prefix-icon="el-icon-search"
                        placeholder="Type to search by asset name"/>
                </el-form-item>
            </el-col>
        </el-row>
    </el-form>
    <el-table
        :data="tableAssets"
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
                    <el-input-number v-model="min_risk" :min="0"></el-input-number>
                </el-form-item>
                <el-form-item label="Max risk">
                    <el-input-number v-model="max_risk" :min="0"></el-input-number>
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
    <el-divider></el-divider>
    <el-form label-position="right" label-width="100px">
        <p>Private inputs:</p>
        <el-form-item label="Weights">
            <el-input v-model="circuitInputs.weights" :readonly="true"/>
        </el-form-item>
        <p>Public inputs:</p>
        <el-form-item label="Risks">
            <el-input v-model="circuitInputs.risks" :readonly="true"/>
        </el-form-item>
        <el-form-item label="Min risk">
            <el-input v-model="circuitInputs.min_risk" :readonly="true"/>
        </el-form-item>
        <el-form-item label="Max risk">
            <el-input v-model="circuitInputs.max_risk" :readonly="true"/>
        </el-form-item>
    </el-form>
    <el-row style="margin-top: 10px;">
        <el-col :span="12">
            <el-button @click="proof" :loading="loading" type="primary">Generate proof</el-button>
        </el-col>
        <el-col :span="12">
            <el-button @click="verify" :loading="loading" type="primary">Verify proof</el-button>
        </el-col>
    </el-row>
    <el-form label-position="top" label-width="100px" class="circuit-outputs">
        <el-row :gutter="20">
            <el-col :span="8">
                <el-form-item label="Proof">
                    <el-input type="textarea" v-model="circuitData.proof" :disabled="loading"></el-input>
                </el-form-item>
            </el-col>
            <el-col :span="8">
                <el-form-item label="Primary input">
                    <el-input type="textarea" v-model="circuitData.primary_input" :disabled="loading"></el-input>
                </el-form-item>
            </el-col>
            <el-col :span="8">
                <el-form-item label="Verification key">
                    <el-input type="textarea" v-model="circuitData.verification_key" :disabled="loading"></el-input>
                </el-form-item>
            </el-col>
        </el-row>
    </el-form>
    <!-- <p>{{ chartSeries }}</p> -->
    <!-- <p>{{ chartOptions }}</p> -->
  </el-main>
</el-container>
</template>

<script>
import { PORTFOLIO } from '~/assets/portfolio'

const DENOMINATOR = 10000;
const sumReducer = (accumulator, curr) => accumulator + curr;

export default {
    components: {
        VueApexCharts: () => import('vue-apexcharts')
    },
    data: function() {
        return {
            assets: PORTFOLIO,
            search: '',
            filterWeighted: false,
            min_risk: 0,
            max_risk: 200,
            riskValidationRules: {
                // aggRisk: [
                //     { required: true, message: 'Please input Activity name', trigger: 'blur' },
                //     { min: 3, max: 5, message: 'Length should be 3 to 5', trigger: 'blur' }
                // ],
                // min_risk: [
                //     { required: true, message: 'Please select minimal risk', trigger: 'change' }
                // ],
                // max_risk: [
                //     { required: true, message: 'Please select maximal risk', trigger: 'change' }
                // ],
            },
            circuitData: {
                proof: '',
                primary_input: '',
                verification_key: '',
            },
            loading: false,
        }
    },
    computed: {
        totalWeight() {
            return this.weightedAssets.map(i => i.weight).reduce(sumReducer);
        },
        aggRisk() {
            if (this.weightedAssets.length === 0) {
                return 0;
            }
            return this.weightedAssets.map(i => i.risk * i.weight / this.totalWeight).reduce(sumReducer).toFixed(3);
        },
        weightedAssets() {
            return this.assets.filter(i => i.weight > 0);
        },
        tableAssets() {
            return this.assets
                .filter(asset => !this.search || asset.symbol.toLowerCase().startsWith(this.search.toLowerCase()))
                .filter(asset => !this.filterWeighted || (this.filterWeighted && asset.weight > 0))
        },
        chartSeries() {
            return this.weightedAssets.map(i => Number((i.weight * i.risk / this.totalWeight).toFixed(3)))
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
        },
        circuitInputs() {
            return {
                weights: this.assets.map(i => parseInt(i.weight / this.totalWeight * DENOMINATOR)),
                risks: this.assets.map(i => i.risk),
                min_risk: this.min_risk * DENOMINATOR,
                max_risk: this.max_risk * DENOMINATOR,
            }
        }
    },
    methods: {
        async proof() {
            this.loading = true;
            try {
                this.circuitData = await this.$axios.$post('/proof/', this.circuitInputs)
                if (this.circuitData.proof.length) {
                    this.$notify({
                        title: 'Success',
                        message: 'Proof was generated!',
                        type: 'success'
                    });
                } else {
                    this.$notify({
                        title: 'Error',
                        message: 'Circuit isn\'t satisfied!',
                        type: 'error'
                    });
                }
            } catch (e) {
                this.$notify({
                    title: 'Error',
                    message: 'Invalid circuit inputs!',
                    type: 'error'
                });
            }
            this.loading = false;

        },
        async verify() {
            this.loading = true;
            const res = await this.$axios.$post('/verify/', this.circuitData)
            this.loading = false;

            if (res.status) {
                this.$notify({
                    title: 'Success',
                    message: 'Proof is valid!',
                    type: 'success'
                });
            } else {
                this.$notify({
                    title: 'Error',
                    message: 'Proof is invalid!',
                    type: 'error'
                });
            }
        }
    },
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
.circuit-outputs textarea {
    height: 250px;
}
</style>
