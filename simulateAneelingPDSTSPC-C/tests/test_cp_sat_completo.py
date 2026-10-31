import importlib.util
import itertools
from pathlib import Path
import unittest
from ortools.sat.python import cp_model

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('complete',ROOT/'cp_sat_completo.py')
complete=importlib.util.module_from_spec(spec);spec.loader.exec_module(complete)

def sample():
    return {'name':'synthetic','coords':{0:(0,0),1:(5,0),2:(4,0)},'fleet':1,
            'tau':{1:{1:8},2:{1:20}},'dist':[[0,10,8],[10,0,2],[8,2,0]],'quantum':4}

class CompleteTest(unittest.TestCase):
    def run_model(self,data,horizon,hint=None):
        model,v=complete.build_model(data,horizon,hint)
        solver=cp_model.CpSolver();solver.parameters.max_time_in_seconds=5;solver.parameters.num_search_workers=1
        status=solver.solve(model)
        return status,complete.extract(data,v,solver.value) if status in (cp_model.OPTIMAL,cp_model.FEASIBLE) else None

    def test_all_assignments_are_free(self):
        status,row=self.run_model(sample(),19)
        self.assertEqual(status,cp_model.OPTIMAL)
        self.assertEqual(row['claimed'],16)
        self.assertEqual(row['route'],[2]);self.assertEqual(row['missions'][0]['client'],1)

    def test_global_infeasibility(self):
        status,_=self.run_model(sample(),15)
        self.assertEqual(status,cp_model.INFEASIBLE)

    def test_hint_does_not_fix_a_bad_assignment(self):
        hint={'route':[1],'missions':[{'client':2,'k':1,'start':0,'end':20}]}
        status,row=self.run_model(sample(),19,hint)
        self.assertEqual(status,cp_model.OPTIMAL);self.assertEqual(row['claimed'],16)

    def test_dominance_is_resource_and_duration_based(self):
        self.assertEqual(complete.nondominated({1:8,2:8,3:7},9),{1:8,3:7})

    def test_depot_and_colocated_clients_keep_individual_coverage(self):
        xy={0:(0,0),1:(2,0),2:(2,0),3:(0,0)}
        data={'name':'synthetic','coords':xy,'fleet':1,'tau':{1:{},2:{1:3},3:{}},
              'dist':[[2*abs(xy[i][0]-xy[j][0]) for j in xy] for i in xy],'quantum':4}
        status,row=self.run_model(data,8)
        self.assertEqual(status,cp_model.OPTIMAL)
        self.assertEqual(row['claimed'],8)
        self.assertEqual(set(row['route'])|{m['client'] for m in row['missions']},{1,2,3})
        self.assertIn(1,row['route']);self.assertIn(3,row['route'])

    def test_cumulative_rules_out_a_workload_only_false_positive(self):
        data={'name':'synthetic','coords':{0:(0,0),1:(5,0),2:(6,0)},'fleet':3,
              'tau':{1:{2:2},2:{2:2}},'dist':[[0,10,12],[10,0,2],[12,2,0]],'quantum':4}
        # Workload=8 <=3*3, mas demanda 2+2 impede paralelismo, exigindo 4.
        status,_=self.run_model(data,3)
        self.assertEqual(status,cp_model.INFEASIBLE)

if __name__=='__main__':unittest.main()
